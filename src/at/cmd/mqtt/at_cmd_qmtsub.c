// src/at/cmd/mqtt/at_cmd_qmtsub.c
#include "at_cmd_qmtsub.h"

#include "at_cmd_structure.h"
#include "esp_err.h"
#include "esp_log.h"

#include <string.h>

static const char* TAG = "AT_CMD_QMTSUB";

// Define the mapping arrays
const enum_str_map_t QMTSUB_RESULT_MAP[QMTSUB_RESULT_MAP_SIZE] = {
    {QMTSUB_RESULT_SUCCESS, "Packet sent successfully and ACK received"},
    {QMTSUB_RESULT_RETRANSMISSION, "Packet retransmission"},
    {QMTSUB_RESULT_FAILED_TO_SEND, "Failed to send a packet"}};

const enum_str_map_t QMTSUB_QOS_MAP[QMTSUB_QOS_MAP_SIZE] = {
    {QMTSUB_QOS_AT_MOST_ONCE, "At most once"},
    {QMTSUB_QOS_AT_LEAST_ONCE, "At least once"},
    {QMTSUB_QOS_EXACTLY_ONCE, "Exactly once"}};

static esp_err_t qmtsub_test_parser(const char* response, void* parsed_data)
{
  if (NULL == response || NULL == parsed_data)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  qmtsub_test_response_t* test_data = (qmtsub_test_response_t*) parsed_data;

  // Initialize output structure
  memset(test_data, 0, sizeof(qmtsub_test_response_t));

  // Find the response start
  const char* start = strstr(response, "+QMTSUB: ");
  if (!start)
  {
    ESP_LOGE(TAG, "No QMTSUB data in test response");
    return ESP_ERR_INVALID_RESPONSE;
  }

  start += 9; // Skip "+QMTSUB: "

  // Response format: +QMTSUB: (range of supported <client_idx>s),(range of supported
  // <msgID>s),<topic>,(range of supported <qos>s)
  int client_min, client_max, msgid_min, msgid_max, qos_min, qos_max;

  // Parse the ranges
  int matched = sscanf(start,
                       "(%d-%d),(%d-%d),\"<topic>\",(%d-%d)",
                       &client_min,
                       &client_max,
                       &msgid_min,
                       &msgid_max,
                       &qos_min,
                       &qos_max);

  if (matched >= 2)
  {
    test_data->client_idx_min = (uint8_t) client_min;
    test_data->client_idx_max = (uint8_t) client_max;

    if (matched >= 4)
    {
      test_data->msgid_min = (uint16_t) msgid_min;
      test_data->msgid_max = (uint16_t) msgid_max;
    }

    if (matched >= 6)
    {
      test_data->qos_min = (uint8_t) qos_min;
      test_data->qos_max = (uint8_t) qos_max;
    }

    return ESP_OK;
  }

  return ESP_ERR_INVALID_RESPONSE;
}

static esp_err_t qmtsub_write_formatter(const void* params, char* buffer, size_t buffer_size)
{
  if (NULL == params || NULL == buffer || 0 == buffer_size)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  const qmtsub_write_params_t* write_params = (const qmtsub_write_params_t*) params;

  // Validate parameters
  if (write_params->client_idx > QMTSUB_CLIENT_IDX_MAX)
  {
    ESP_LOGE(TAG,
             "Invalid client_idx: %d (must be 0-%d)",
             write_params->client_idx,
             QMTSUB_CLIENT_IDX_MAX);
    return ESP_ERR_INVALID_ARG;
  }

  if (write_params->msgid < QMTSUB_MSGID_MIN || write_params->msgid > QMTSUB_MSGID_MAX)
  {
    ESP_LOGE(TAG,
             "Invalid msgid: %d (must be %d-%d)",
             write_params->msgid,
             QMTSUB_MSGID_MIN,
             QMTSUB_MSGID_MAX);
    return ESP_ERR_INVALID_ARG;
  }

  if (write_params->topic_count == 0 || write_params->topic_count > QMTSUB_MAX_TOPICS)
  {
    ESP_LOGE(TAG,
             "Invalid topic_count: %d (must be 1-%d)",
             write_params->topic_count,
             QMTSUB_MAX_TOPICS);
    return ESP_ERR_INVALID_ARG;
  }

  // Format the command
  int written =
      snprintf(buffer, buffer_size, "=%d,%d", write_params->client_idx, write_params->msgid);

  if (written < 0 || (size_t) written >= buffer_size)
  {
    ESP_LOGE(TAG, "Buffer too small for QMTSUB command");
    if (buffer_size > 0)
    {
      buffer[0] = '\0'; // Ensure null-terminated in case of overflow
    }
    return ESP_ERR_INVALID_SIZE;
  }

  // Add each topic and QoS pair
  for (uint8_t i = 0; i < write_params->topic_count; i++)
  {
    const qmtsub_topic_filter_t* topic = &write_params->topics[i];

    // Validate QoS
    if (topic->qos > QMTSUB_QOS_EXACTLY_ONCE)
    {
      ESP_LOGE(TAG, "Invalid QoS: %d for topic %d", topic->qos, i);
      return ESP_ERR_INVALID_ARG;
    }

    // Validate topic is not empty
    if (topic->topic[0] == '\0')
    {
      ESP_LOGE(TAG, "Empty topic string for topic %d", i);
      return ESP_ERR_INVALID_ARG;
    }

    // Format topic and QoS
    int topic_written =
        snprintf(buffer + written, buffer_size - written, ",\"%s\",%d", topic->topic, topic->qos);

    if (topic_written < 0 || (size_t) (written + topic_written) >= buffer_size)
    {
      ESP_LOGE(TAG, "Buffer too small for QMTSUB command topics");
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

static esp_err_t qmtsub_write_parser(const char* response, void* parsed_data)
{
  if (NULL == response || NULL == parsed_data)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  qmtsub_write_response_t* write_resp = (qmtsub_write_response_t*) parsed_data;

  // Initialize output structure
  memset(write_resp, 0, sizeof(qmtsub_write_response_t));

  // URC response format: +QMTSUB: <client_idx>,<msgID>,<result>[,<value>]
  const char* result_start = strstr(response, "+QMTSUB: ");
  if (!result_start)
  {
    // The response might not contain the URC yet (just the OK)
    // This is normal because the URC might come later
    return ESP_OK;
  }

  result_start += 9; // Skip "+QMTSUB: "

  int client_idx, msgid, result, value = 0;
  int matched = sscanf(result_start, "%d,%d,%d,%d", &client_idx, &msgid, &result, &value);

  if (matched >= 3) // At least client_idx, msgid, and result are required
  {
    if (client_idx >= QMTSUB_CLIENT_IDX_MIN && client_idx <= QMTSUB_CLIENT_IDX_MAX)
    {
      write_resp->client_idx             = (uint8_t) client_idx;
      write_resp->present.has_client_idx = true;
    }
    else
    {
      ESP_LOGW(TAG, "Invalid client_idx in response: %d", client_idx);
    }

    if (msgid >= QMTSUB_MSGID_MIN && msgid <= QMTSUB_MSGID_MAX)
    {
      write_resp->msgid             = (uint16_t) msgid;
      write_resp->present.has_msgid = true;
    }
    else
    {
      ESP_LOGW(TAG, "Invalid msgid in response: %d", msgid);
    }

    // Set the result code
    if (result >= QMTSUB_RESULT_SUCCESS && result <= QMTSUB_RESULT_FAILED_TO_SEND)
    {
      write_resp->result             = (qmtsub_result_t) result;
      write_resp->present.has_result = true;
    }
    else
    {
      ESP_LOGW(TAG, "Invalid result in response: %d", result);
    }

    // If value is present
    if (matched >= 4)
    {
      write_resp->value             = (uint8_t) value;
      write_resp->present.has_value = true;

      if (result == QMTSUB_RESULT_SUCCESS)
      {
        ESP_LOGI(TAG, "Granted QoS level: %d", value);
      }
      else if (result == QMTSUB_RESULT_RETRANSMISSION)
      {
        ESP_LOGI(TAG, "Retransmission count: %d", value);
      }
    }

    // Log result for debugging
    ESP_LOGI(TAG,
             "QMTSUB operation result: %d (%s)",
             result,
             enum_to_str(result, QMTSUB_RESULT_MAP, QMTSUB_RESULT_MAP_SIZE));

    return ESP_OK;
  }

  return ESP_ERR_INVALID_RESPONSE;
}

// Command definition for QMTSUB
const at_cmd_t AT_CMD_QMTSUB = {
    .name        = "QMTSUB",
    .description = "Subscribe to Topics",
    .type_info   = {[AT_CMD_TYPE_TEST]    = {.parser        = qmtsub_test_parser,
                                             .formatter     = NULL,
                                             .response_type = AT_CMD_RESPONSE_TYPE_DATA_REQUIRED},
                    [AT_CMD_TYPE_READ]    = AT_CMD_TYPE_DOES_NOT_EXIST,
                    [AT_CMD_TYPE_WRITE]   = {.parser        = qmtsub_write_parser,
                                             .formatter     = qmtsub_write_formatter,
                                             .response_type = AT_CMD_RESPONSE_TYPE_DATA_REQUIRED},
                    [AT_CMD_TYPE_EXECUTE] = AT_CMD_TYPE_DOES_NOT_EXIST},
    .timeout_ms  = 15000 // Default is packet_timeout × retry_times (default: 15s)
};
