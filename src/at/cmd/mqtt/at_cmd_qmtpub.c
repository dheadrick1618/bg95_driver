#include "at_cmd_qmtpub.h"

#include "at_cmd_structure.h"
#include "esp_err.h"
#include "esp_log.h"

#include <string.h>

static const char* TAG = "AT_CMD_QMTPUB";

// Define the mapping arrays
const enum_str_map_t QMTPUB_RESULT_MAP[QMTPUB_RESULT_MAP_SIZE] = {
    {QMTPUB_RESULT_SUCCESS, "Packet sent successfully and ACK received"},
    {QMTPUB_RESULT_RETRANSMISSION, "Packet retransmission"},
    {QMTPUB_RESULT_FAILED_TO_SEND, "Failed to send a packet"}};

const enum_str_map_t QMTPUB_QOS_MAP[QMTPUB_QOS_MAP_SIZE] = {
    {QMTPUB_QOS_AT_MOST_ONCE, "At most once"},
    {QMTPUB_QOS_AT_LEAST_ONCE, "At least once"},
    {QMTPUB_QOS_EXACTLY_ONCE, "Exactly once"}};

const enum_str_map_t QMTPUB_RETAIN_MAP[QMTPUB_RETAIN_MAP_SIZE] = {
    {QMTPUB_RETAIN_DISABLED, "Do not retain"}, {QMTPUB_RETAIN_ENABLED, "Retain message"}};

static esp_err_t qmtpub_write_formatter(const void* params, char* buffer, size_t buffer_size)
{
  if (NULL == params || NULL == buffer || 0 == buffer_size)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  const qmtpub_write_params_t* write_params = (const qmtpub_write_params_t*) params;

  // Validate parameters
  if (write_params->client_idx > QMTPUB_CLIENT_IDX_MAX)
  {
    ESP_LOGE(TAG,
             "Invalid client_idx: %d (must be 0-%d)",
             write_params->client_idx,
             QMTPUB_CLIENT_IDX_MAX);
    return ESP_ERR_INVALID_ARG;
  }

  if (write_params->msgid > QMTPUB_MSGID_MAX)
  {
    ESP_LOGE(TAG, "Invalid msgid: %d (must be 0-%d)", write_params->msgid, QMTPUB_MSGID_MAX);
    return ESP_ERR_INVALID_ARG;
  }

  if (write_params->qos > QMTPUB_QOS_EXACTLY_ONCE)
  {
    ESP_LOGE(TAG, "Invalid QoS level: %d", write_params->qos);
    return ESP_ERR_INVALID_ARG;
  }

  if (write_params->retain > QMTPUB_RETAIN_ENABLED)
  {
    ESP_LOGE(TAG, "Invalid retain flag: %d", write_params->retain);
    return ESP_ERR_INVALID_ARG;
  }

  if (write_params->topic[0] == '\0')
  {
    ESP_LOGE(TAG, "Topic cannot be empty");
    return ESP_ERR_INVALID_ARG;
  }

  if (write_params->msglen < 1 || write_params->msglen > QMTPUB_MSG_MAX_LEN)
  {
    ESP_LOGE(
        TAG, "Invalid message length: %d (must be 1-%d)", write_params->msglen, QMTPUB_MSG_MAX_LEN);
    return ESP_ERR_INVALID_ARG;
  }

  int written = snprintf(buffer,
                         buffer_size,
                         "=%d,%d,%d,%d,\"%s\",%d",
                         write_params->client_idx,
                         write_params->msgid,
                         write_params->qos,
                         write_params->retain,
                         write_params->topic,
                         write_params->msglen);

  // Check for buffer overflow
  if (written < 0 || (size_t) written >= buffer_size)
  {
    ESP_LOGE(TAG, "Buffer too small for QMTPUB command");
    if (buffer_size > 0)
    {
      buffer[0] = '\0'; // Ensure null-terminated in case of overflow
    }
    return ESP_ERR_INVALID_SIZE;
  }

  return ESP_OK;
}

static esp_err_t qmtpub_write_parser(const char* response, void* parsed_data)
{
  if (NULL == response || NULL == parsed_data)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  qmtpub_write_response_t* write_resp = (qmtpub_write_response_t*) parsed_data;

  // Initialize output structure
  memset(write_resp, 0, sizeof(qmtpub_write_response_t));

  // URC response format: +QMTPUB: <client_idx>,<msgID>,<result>[,<value>]
  const char* result_start = strstr(response, "+QMTPUB: ");
  if (!result_start)
  {
    // The response might not contain the URC yet (just the OK)
    // This is normal because the URC might come later
    return ESP_OK;
  }

  result_start += 9; // Skip "+QMTPUB: "

  int client_idx, msgid, result, value;
  int matched = sscanf(result_start, "%d,%d,%d,%d", &client_idx, &msgid, &result, &value);

  if (matched >= 3) // At least client_idx, msgid, and result are required
  {
    if (client_idx >= QMTPUB_CLIENT_IDX_MIN && client_idx <= QMTPUB_CLIENT_IDX_MAX)
    {
      write_resp->client_idx             = (uint8_t) client_idx;
      write_resp->present.has_client_idx = true;
    }
    else
    {
      ESP_LOGW(TAG, "Invalid client_idx in response: %d", client_idx);
    }

    if (msgid >= QMTPUB_MSGID_MIN && msgid <= QMTPUB_MSGID_MAX)
    {
      write_resp->msgid             = (uint16_t) msgid;
      write_resp->present.has_msgid = true;
    }
    else
    {
      ESP_LOGW(TAG, "Invalid msgid in response: %d", msgid);
    }

    // Set the result code
    if (result >= QMTPUB_RESULT_SUCCESS && result <= QMTPUB_RESULT_FAILED_TO_SEND)
    {
      write_resp->result             = (qmtpub_result_t) result;
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
    }

    // Log result for debugging
    ESP_LOGI(TAG,
             "QMTPUB operation result: %d (%s)",
             result,
             enum_to_str(result, QMTPUB_RESULT_MAP, QMTPUB_RESULT_MAP_SIZE));

    if (write_resp->present.has_value && result == QMTPUB_RESULT_RETRANSMISSION)
    {
      ESP_LOGI(TAG, "Retransmission count: %d", write_resp->value);
    }

    return ESP_OK;
  }

  return ESP_ERR_INVALID_RESPONSE;
}

// Command definition for QMTPUB
const at_cmd_t AT_CMD_QMTPUB = {
    .name        = "QMTPUB",
    .description = "Publish Messages to MQTT Server",
    .type_info   = {[AT_CMD_TYPE_TEST]    = AT_CMD_TYPE_NOT_IMPLEMENTED,
                    [AT_CMD_TYPE_READ]    = AT_CMD_TYPE_DOES_NOT_EXIST,
                    [AT_CMD_TYPE_WRITE]   = {.parser        = qmtpub_write_parser,
                                             .formatter     = qmtpub_write_formatter,
                                             .response_type = AT_CMD_RESPONSE_TYPE_DATA_REQUIRED},
                    [AT_CMD_TYPE_EXECUTE] = AT_CMD_TYPE_DOES_NOT_EXIST},
    .timeout_ms  = 15000 // Default is pkt_timeout × retry_times (default 15s)
};
