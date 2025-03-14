// components/bg95_driver/src/at/cmd/mqtt/at_cmd_qmtopen.c

#include "at_cmd_qmtopen.h"

#include "at_cmd_structure.h"
#include "esp_err.h"
#include "esp_log.h"

#include <string.h>

static const char* TAG = "AT_CMD_QMTOPEN";

// Define the result code mapping array
const enum_str_map_t QMTOPEN_RESULT_MAP[QMTOPEN_RESULT_MAP_SIZE] = {
    {QMTOPEN_RESULT_FAILED_TO_OPEN, "Failed to open network"},
    {QMTOPEN_RESULT_OPEN_SUCCESS, "Network opened successfully"},
    {QMTOPEN_RESULT_WRONG_PARAMETER, "Wrong parameter"},
    {QMTOPEN_RESULT_MQTT_ID_OCCUPIED, "MQTT client identifier is occupied"},
    {QMTOPEN_RESULT_FAILED_ACTIVATE_PDP, "Failed to activate PDP"},
    {QMTOPEN_RESULT_FAILED_PARSE_DOMAIN, "Failed to parse domain name"},
    {QMTOPEN_RESULT_NETWORK_CONN_ERROR, "Network connection error"}};

static esp_err_t qmtopen_read_parser(const char* response, void* parsed_data)
{
  if (NULL == response || NULL == parsed_data)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  qmtopen_read_response_t* read_data = (qmtopen_read_response_t*) parsed_data;

  // Initialize output structure
  memset(read_data, 0, sizeof(qmtopen_read_response_t));

  // Find the response start
  const char* start = strstr(response, "+QMTOPEN: ");
  if (!start)
  {
    // No QMTOPEN data in response (might just be OK without data)
    return ESP_OK;
  }

  start += 10; // Skip "+QMTOPEN: "

  // Parse client_idx, host_name, and port
  int  client_idx, port;
  char host_name[QMTOPEN_HOST_NAME_MAX_SIZE + 1] = {0};

  // Format: +QMTOPEN: <client_idx>,<host_name>,<port>
  int matched = sscanf(start, "%d,\"%100[^\"]\",%d", &client_idx, host_name, &port);

  if (matched >= 1)
  {
    if (client_idx >= QMTOPEN_CLIENT_IDX_MIN && client_idx <= QMTOPEN_CLIENT_IDX_MAX)
    {
      read_data->client_idx             = (uint8_t) client_idx;
      read_data->present.has_client_idx = true;
    }
    else
    {
      ESP_LOGW(TAG, "Invalid client_idx in response: %d", client_idx);
    }
  }

  if (matched >= 2)
  {
    strncpy(read_data->host_name, host_name, sizeof(read_data->host_name) - 1);
    read_data->host_name[sizeof(read_data->host_name) - 1] = '\0'; // Ensure null termination
    read_data->present.has_host_name                       = true;
  }

  if (matched >= 3)
  {
    if (port >= QMTOPEN_PORT_MIN && port <= QMTOPEN_PORT_MAX)
    {
      read_data->port             = (uint16_t) port;
      read_data->present.has_port = true;
    }
    else
    {
      ESP_LOGW(TAG, "Invalid port in response: %d", port);
    }
  }

  return (matched >= 1) ? ESP_OK : ESP_ERR_INVALID_RESPONSE;
}

static esp_err_t qmtopen_write_formatter(const void* params, char* buffer, size_t buffer_size)
{
  if (NULL == params || NULL == buffer || 0 == buffer_size)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  const qmtopen_write_params_t* write_params = (const qmtopen_write_params_t*) params;

  // Validate parameters
  if (write_params->client_idx > QMTOPEN_CLIENT_IDX_MAX)
  {
    ESP_LOGE(TAG,
             "Invalid client_idx: %d (must be 0-%d)",
             write_params->client_idx,
             QMTOPEN_CLIENT_IDX_MAX);
    return ESP_ERR_INVALID_ARG;
  }

  if (write_params->port > QMTOPEN_PORT_MAX)
  {
    ESP_LOGE(TAG, "Invalid port: %d (must be 0-%d)", write_params->port, QMTOPEN_PORT_MAX);
    return ESP_ERR_INVALID_ARG;
  }

  if (write_params->host_name[0] == '\0')
  {
    ESP_LOGE(TAG, "Empty host name is not allowed");
    return ESP_ERR_INVALID_ARG;
  }

  // Format the command
  int written = snprintf(buffer,
                         buffer_size,
                         "=%d,\"%s\",%d",
                         write_params->client_idx,
                         write_params->host_name,
                         write_params->port);

  // Check for buffer overflow
  if (written < 0 || (size_t) written >= buffer_size)
  {
    ESP_LOGE(TAG, "Buffer too small for QMTOPEN write command");
    if (buffer_size > 0)
    {
      buffer[0] = '\0'; // Ensure null-terminated in case of overflow
    }
    return ESP_ERR_INVALID_SIZE;
  }

  return ESP_OK;
}

static esp_err_t qmtopen_write_parser(const char* response, void* parsed_data)
{
  if (NULL == response || NULL == parsed_data)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  qmtopen_write_response_t* write_resp = (qmtopen_write_response_t*) parsed_data;

  // Initialize output structure
  memset(write_resp, 0, sizeof(qmtopen_write_response_t));

  // URC response format: +QMTOPEN: <client_idx>,<result>
  const char* result_start = strstr(response, "+QMTOPEN: ");
  if (!result_start)
  {
    // The response might not contain the URC yet (just the OK)
    // This is normal because the URC might come later
    return ESP_OK;
  }

  result_start += 10; // Skip "+QMTOPEN: "

  int client_idx, result;
  if (sscanf(result_start, "%d,%d", &client_idx, &result) == 2)
  {
    if (client_idx >= QMTOPEN_CLIENT_IDX_MIN && client_idx <= QMTOPEN_CLIENT_IDX_MAX)
    {
      write_resp->client_idx             = (uint8_t) client_idx;
      write_resp->present.has_client_idx = true;
    }
    else
    {
      ESP_LOGW(TAG, "Invalid client_idx in response: %d", client_idx);
    }

    // Set the result code
    write_resp->result             = (qmtopen_result_t) result;
    write_resp->present.has_result = true;

    // Log result for debugging
    ESP_LOGI(TAG,
             "QMTOPEN operation result: %d (%s)",
             result,
             enum_to_str(result, QMTOPEN_RESULT_MAP, QMTOPEN_RESULT_MAP_SIZE));

    return ESP_OK;
  }

  return ESP_ERR_INVALID_RESPONSE;
}

// Command definition for QMTOPEN
const at_cmd_t AT_CMD_QMTOPEN = {
    .name        = "QMTOPEN",
    .description = "Open a Network Connection for MQTT Client",
    .type_info   = {[AT_CMD_TYPE_TEST] =
                        AT_CMD_TYPE_NOT_IMPLEMENTED, // As requested, skipping test command
                    [AT_CMD_TYPE_READ]    = {.parser = qmtopen_read_parser, .formatter = NULL},
                    [AT_CMD_TYPE_WRITE]   = {.parser    = qmtopen_write_parser,
                                             .formatter = qmtopen_write_formatter},
                    [AT_CMD_TYPE_EXECUTE] = AT_CMD_TYPE_DOES_NOT_EXIST},
    .timeout_ms  = 120000 // 120 seconds (network operations can take time)
};
