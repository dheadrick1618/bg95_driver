// at_cmd_qmtdisc.c
#include "at_cmd_qmtdisc.h"

#include "at_cmd_structure.h"
#include "esp_err.h"
#include "esp_log.h"

#include <string.h>

static const char* TAG = "AT_CMD_QMTDISC";

// Define the result code mapping array
const enum_str_map_t QMTDISC_RESULT_MAP[QMTDISC_RESULT_MAP_SIZE] = {
    {QMTDISC_RESULT_SUCCESS, "Disconnect message sent successfully"},
    {QMTDISC_RESULT_FAILED_TO_SEND, "Failed to send disconnect message"}};

static esp_err_t qmtdisc_test_parser(const char* response, void* parsed_data)
{
  if (NULL == response || NULL == parsed_data)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  qmtdisc_test_response_t* test_data = (qmtdisc_test_response_t*) parsed_data;

  // Initialize output structure
  memset(test_data, 0, sizeof(qmtdisc_test_response_t));

  // Find the response start
  const char* start = strstr(response, "+QMTDISC: ");
  if (!start)
  {
    ESP_LOGE(TAG, "No QMTDISC data in test response");
    return ESP_ERR_INVALID_RESPONSE;
  }

  start += 10; // Skip "+QMTDISC: "

  // Parse the range of supported client_idx values
  // The format is expected to be: "(min-max)"
  int min_idx, max_idx;
  if (sscanf(start, "(%d-%d)", &min_idx, &max_idx) == 2)
  {
    test_data->client_idx_min = (uint8_t) min_idx;
    test_data->client_idx_max = (uint8_t) max_idx;
    return ESP_OK;
  }

  return ESP_ERR_INVALID_RESPONSE;
}

static esp_err_t qmtdisc_write_formatter(const void* params, char* buffer, size_t buffer_size)
{
  if (NULL == params || NULL == buffer || 0 == buffer_size)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  const qmtdisc_write_params_t* write_params = (const qmtdisc_write_params_t*) params;

  // Validate parameters
  if (write_params->client_idx > QMTDISC_CLIENT_IDX_MAX)
  {
    ESP_LOGE(TAG,
             "Invalid client_idx: %d (must be 0-%d)",
             write_params->client_idx,
             QMTDISC_CLIENT_IDX_MAX);
    return ESP_ERR_INVALID_ARG;
  }

  // Format the command
  int written = snprintf(buffer, buffer_size, "=%d", write_params->client_idx);

  // Check for buffer overflow
  if (written < 0 || (size_t) written >= buffer_size)
  {
    ESP_LOGE(TAG, "Buffer too small for QMTDISC write command");
    if (buffer_size > 0)
    {
      buffer[0] = '\0'; // Ensure null-terminated in case of overflow
    }
    return ESP_ERR_INVALID_SIZE;
  }

  return ESP_OK;
}

static esp_err_t qmtdisc_write_parser(const char* response, void* parsed_data)
{
  if (NULL == response || NULL == parsed_data)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  qmtdisc_write_response_t* write_resp = (qmtdisc_write_response_t*) parsed_data;

  // Initialize output structure
  memset(write_resp, 0, sizeof(qmtdisc_write_response_t));

  // URC response format: +QMTDISC: <client_idx>,<result>
  const char* result_start = strstr(response, "+QMTDISC: ");
  if (!result_start)
  {
    // The response might not contain the URC yet (just the OK)
    // This is normal because the URC might come later
    return ESP_OK;
  }

  result_start += 10; // Skip "+QMTDISC: "

  int client_idx, result;
  if (sscanf(result_start, "%d,%d", &client_idx, &result) == 2)
  {
    if (client_idx >= QMTDISC_CLIENT_IDX_MIN && client_idx <= QMTDISC_CLIENT_IDX_MAX)
    {
      write_resp->client_idx             = (uint8_t) client_idx;
      write_resp->present.has_client_idx = true;
    }
    else
    {
      ESP_LOGW(TAG, "Invalid client_idx in response: %d", client_idx);
    }

    // Set the result code
    write_resp->result             = (qmtdisc_result_t) result;
    write_resp->present.has_result = true;

    // Log result for debugging
    ESP_LOGI(TAG,
             "QMTDISC operation result: %d (%s)",
             result,
             enum_to_str(result, QMTDISC_RESULT_MAP, QMTDISC_RESULT_MAP_SIZE));

    return ESP_OK;
  }

  return ESP_ERR_INVALID_RESPONSE;
}

// Command definition for QMTDISC
const at_cmd_t AT_CMD_QMTDISC = {
    .name        = "QMTDISC",
    .description = "Disconnect a Client from MQTT Server",
    .type_info   = {[AT_CMD_TYPE_TEST]    = {.parser = qmtdisc_test_parser, .formatter = NULL},
                    [AT_CMD_TYPE_READ]    = AT_CMD_TYPE_DOES_NOT_EXIST,
                    [AT_CMD_TYPE_WRITE]   = {.parser    = qmtdisc_write_parser,
                                             .formatter = qmtdisc_write_formatter},
                    [AT_CMD_TYPE_EXECUTE] = AT_CMD_TYPE_DOES_NOT_EXIST},
    .timeout_ms  = 5000 // 5 seconds, based on the "default 5s" in the spec
};
