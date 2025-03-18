#include "at_cmd_qmtuns.h"

#include "at_cmd_structure.h"
#include "esp_err.h"
#include "esp_log.h"

#include <string.h>

static const char* TAG = "AT_CMD_QMTUNS";

// Define the mapping arrays
const enum_str_map_t QMTUNS_RESULT_MAP[QMTUNS_RESULT_MAP_SIZE] = {
    {QMTUNS_RESULT_SUCCESS, "Packet sent successfully and ACK received"},
    {QMTUNS_RESULT_RETRANSMISSION, "Packet retransmission"},
    {QMTUNS_RESULT_FAILED_TO_SEND, "Failed to send a packet"}};

static esp_err_t qmtuns_test_parser(const char* response, void* parsed_data)
{
  if (NULL == response || NULL == parsed_data)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  qmtuns_test_response_t* test_data = (qmtuns_test_response_t*) parsed_data;

  // Initialize output structure
  memset(test_data, 0, sizeof(qmtuns_test_response_t));

  // Find the response start
  const char* start = strstr(response, "+QMTUNS: ");
  if (!start)
  {
    ESP_LOGE(TAG, "No QMTUNS data in test response");
    return ESP_ERR_INVALID_RESPONSE;
  }

  start += 9; // Skip "+QMTUNS: "

  // Response format: +QMTUNS: (range of supported <client_idx>s),(range of supported
  // <msgID>s),<topic>
  int client_min, client_max, msgid_min, msgid_max;

  // Parse the ranges
  int matched = sscanf(start, "(%d-%d),(%d-%d)", &client_min, &client_max, &msgid_min, &msgid_max);

  if (matched >= 2)
  {
    test_data->client_idx_min = (uint8_t) client_min;
    test_data->client_idx_max = (uint8_t) client_max;

    if (matched >= 4)
    {
      test_data->msgid_min = (uint16_t) msgid_min;
      test_data->msgid_max = (uint16_t) msgid_max;
    }

    return ESP_OK;
  }

  return ESP_ERR_INVALID_RESPONSE;
}

static esp_err_t qmtuns_write_formatter(const void* params, char* buffer, size_t buffer_size)
{
  if (NULL == params || NULL == buffer || 0 == buffer_size)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  const qmtuns_write_params_t* write_params = (const qmtuns_write_params_t*) params;

  // Validate parameters
  if (write_params->client_idx > QMTUNS_CLIENT_IDX_MAX)
  {
    ESP_LOGE(TAG,
             "Invalid client_idx: %d (must be 0-%d)",
             write_params->client_idx,
             QMTUNS_CLIENT_IDX_MAX);
    return ESP_ERR_INVALID_ARG;
  }

  if (write_params->msgid < QMTUNS_MSGID_MIN || write_params->msgid > QMTUNS_MSGID_MAX)
  {
    ESP_LOGE(TAG,
             "Invalid msgid: %d (must be %d-%d)",
             write_params->msgid,
             QMTUNS_MSGID_MIN,
             QMTUNS_MSGID_MAX);
    return ESP_ERR_INVALID_ARG;
  }

  if (write_params->topic_count == 0 || write_params->topic_count > QMTUNS_MAX_TOPICS)
  {
    ESP_LOGE(TAG,
             "Invalid topic_count: %d (must be 1-%d)",
             write_params->topic_count,
             QMTUNS_MAX_TOPICS);
    return ESP_ERR_INVALID_ARG;
  }

  // Format the command
  int written =
      snprintf(buffer, buffer_size, "=%d,%d", write_params->client_idx, write_params->msgid);

  if (written < 0 || (size_t) written >= buffer_size)
  {
    ESP_LOGE(TAG, "Buffer too small for QMTUNS command");
    if (buffer_size > 0)
    {
      buffer[0] = '\0'; // Ensure null-terminated in case of overflow
    }
    return ESP_ERR_INVALID_SIZE;
  }

  // Add each topic
  for (uint8_t i = 0; i < write_params->topic_count; i++)
  {
    // Validate topic is not empty
    if (write_params->topics[i][0] == '\0')
    {
      ESP_LOGE(TAG, "Empty topic string for topic %d", i);
      return ESP_ERR_INVALID_ARG;
    }

    // Format topic
    int topic_written =
        snprintf(buffer + written, buffer_size - written, ",\"%s\"", write_params->topics[i]);

    if (topic_written < 0 || (size_t) (written + topic_written) >= buffer_size)
    {
      ESP_LOGE(TAG, "Buffer too small for QMTUNS command topics");
      if (buffer_size > 0)
      {
        buffer[0] = '\0';
      }
      return ESP_ERR_INVALID_SIZE;
    }

    written += topic_written;
  }

  return ESP_OK;
}

static esp_err_t qmtuns_write_parser(const char* response, void* parsed_data)
{
  if (NULL == response || NULL == parsed_data)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  qmtuns_write_response_t* write_resp = (qmtuns_write_response_t*) parsed_data;

  // Initialize output structure
  memset(write_resp, 0, sizeof(qmtuns_write_response_t));

  // URC response format: +QMTUNS: <client_idx>,<msgID>,<result>
  const char* result_start = strstr(response, "+QMTUNS: ");
  if (!result_start)
  {
    // The response might not contain the URC yet (just the OK)
    // This is normal because the URC might come later
    return ESP_OK;
  }

  result_start += 9; // Skip "+QMTUNS: "

  int client_idx, msgid, result;
  int matched = sscanf(result_start, "%d,%d,%d", &client_idx, &msgid, &result);

  if (matched >= 3) // We need all three values
  {
    if (client_idx >= QMTUNS_CLIENT_IDX_MIN && client_idx <= QMTUNS_CLIENT_IDX_MAX)
    {
      write_resp->client_idx             = (uint8_t) client_idx;
      write_resp->present.has_client_idx = true;
    }
    else
    {
      ESP_LOGW(TAG, "Invalid client_idx in response: %d", client_idx);
    }

    if (msgid >= QMTUNS_MSGID_MIN && msgid <= QMTUNS_MSGID_MAX)
    {
      write_resp->msgid             = (uint16_t) msgid;
      write_resp->present.has_msgid = true;
    }
    else
    {
      ESP_LOGW(TAG, "Invalid msgid in response: %d", msgid);
    }

    // Set the result code
    if (result >= QMTUNS_RESULT_SUCCESS && result <= QMTUNS_RESULT_FAILED_TO_SEND)
    {
      write_resp->result             = (qmtuns_result_t) result;
      write_resp->present.has_result = true;
    }
    else
    {
      ESP_LOGW(TAG, "Invalid result in response: %d", result);
    }

    // Log result for debugging
    ESP_LOGI(TAG,
             "QMTUNS operation result: %d (%s)",
             result,
             enum_to_str(result, QMTUNS_RESULT_MAP, QMTUNS_RESULT_MAP_SIZE));

    return ESP_OK;
  }

  return ESP_ERR_INVALID_RESPONSE;
}

// Command definition for QMTUNS
const at_cmd_t AT_CMD_QMTUNS = {
    .name        = "QMTUNS",
    .description = "Unsubscribe from Topics",
    .type_info   = {[AT_CMD_TYPE_TEST]    = {.parser        = qmtuns_test_parser,
                                             .formatter     = NULL,
                                             .response_type = AT_CMD_RESPONSE_TYPE_DATA_REQUIRED},
                    [AT_CMD_TYPE_READ]    = AT_CMD_TYPE_DOES_NOT_EXIST,
                    [AT_CMD_TYPE_WRITE]   = {.parser        = qmtuns_write_parser,
                                             .formatter     = qmtuns_write_formatter,
                                             .response_type = AT_CMD_RESPONSE_TYPE_DATA_REQUIRED},
                    [AT_CMD_TYPE_EXECUTE] = AT_CMD_TYPE_DOES_NOT_EXIST},
    .timeout_ms  = 15000 // Default is packet_timeout × retry_times (default: 15s)
};
