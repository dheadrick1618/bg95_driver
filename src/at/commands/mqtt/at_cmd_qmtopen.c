
#include "mqtt/at_cmd_qmtopen.h"

#include "at_cmd_structure.h"
#include "esp_err.h"

#include <esp_log.h>
#include <string.h>

static const char* TAG = "AT_CMD_QMTOPEN";

static esp_err_t qmtopen_read_parser(const char* response, void* parsed_data)
{
  qmtopen_read_response_t* read_data = (qmtopen_read_response_t*) parsed_data;
  memset(read_data, 0, sizeof(qmtopen_read_response_t));

  // Find response start
  const char* start = strstr(response, "+QMTOPEN: ");
  if (!start)
  {
    return ESP_ERR_INVALID_RESPONSE;
  }

  start += 10; // shift start index over to where response data begins

  // uint8_t client_id;
  int  client_id;
  char host_name[128];
  // uint16_t port;
  int port;

  int matched = sscanf(start, "%d,\"%127[^\"]\",%d", &client_id, host_name, &port);

  if (matched == 3)
  {
    // validate range
    if ((client_id < 0) || (client_id) > 5)
    {
      return ESP_ERR_INVALID_RESPONSE;
    }
    if ((port > 1) || (port > 65535))
    {
      return ESP_ERR_INVALID_RESPONSE;
    }

    read_data->client_id = (uint8_t) client_id;
    strncpy(read_data->host_name, host_name, sizeof(read_data->host_name) - 1);
    read_data->host_name[sizeof(read_data->host_name) - 1] = '\0';
    read_data->port                                        = (uint16_t) port;

    return ESP_OK;
  }

  // creg_read_response_t* read_data = (creg_read_response_t*)parsed_data;
  // memset(read_data, 0, sizeof(creg_read_response_t));
  //
  // // Find response start
  // const char* start = strstr(response, "+CREG: ");
  // if (!start) {
  //     return ESP_ERR_INVALID_RESPONSE;
  // }
  // start += 7;
  //
  // int n, stat;
  // char lac[5] = {0}, ci[9] = {0};
  // int act = -1;
  //
  // // Try to parse with all possible formats
  // int matched = sscanf(start, "%d,%d,\"%4[^\"]\",%8[^\"]\",%d",
  //                     &n, &stat, lac, ci, &act);
  //
  // if (matched >= 2) {  // At minimum need n and stat
  //     read_data->n = n;
  //     read_data->present.has_n = true;
  //
  //     if (stat >= 0 && stat < CREG_STATUS_MAX) {
  //         read_data->status = (creg_status_t)stat;
  //         read_data->present.has_status = true;
  //     }
  //
  //     if (matched >= 3 && strlen(lac) > 0) {
  //         strncpy(read_data->lac, lac, sizeof(read_data->lac)-1);
  //         read_data->present.has_lac = true;
  //     }
  //
  //     if (matched >= 4 && strlen(ci) > 0) {
  //         strncpy(read_data->ci, ci, sizeof(read_data->ci)-1);
  //         read_data->present.has_ci = true;
  //     }
  //
  //     if (matched >= 5 && act >= 0) {
  //         read_data->act = (creg_act_t)act;
  //         read_data->present.has_act = true;
  //     }
  //
  //     return ESP_OK;
  // }

  return ESP_ERR_INVALID_RESPONSE;
}

static esp_err_t qmtopen_write_formatter(const void* params, char* buffer, size_t buffer_size)
{
  if ((NULL == params) || (NULL == buffer) || (0U == buffer_size))
  {
    ESP_LOGE(TAG, "Invalid formatter NULL args");
    return ESP_ERR_INVALID_ARG;
  }

  ESP_LOGI("AT_CMD_QMTOPEN", "Raw params pointer: %p", params);
  const qmtopen_write_params_t* write_params = (const qmtopen_write_params_t*) params;
  ESP_LOGI("AT_CMD_QMTOPEN", "Write params at start: %p", write_params);

  ESP_LOGI("AT_CMD_QMTOPEN",
           "Write params: client_id=%u, host=%s, port=%u",
           write_params->client_id,
           write_params->host_name,
           write_params->port);

  // Validate parameters
  if (write_params->client_id > 5 || write_params->client_id < 0)
  {
    ESP_LOGE("AT_CMD_QMTOPEN", "Invalid client_id: %d", write_params->client_id);
    return ESP_ERR_INVALID_ARG;
  }

  if (write_params->port <= 0 || write_params->port > 65535)
  {
    ESP_LOGE("AT_CMD_QMTOPEN", "Invalid port: %d", write_params->port);
    return ESP_ERR_INVALID_ARG;
  }
  if (write_params->host_name[0] == '\0')
  {

    ESP_LOGE(TAG, "Invalid formatter host name arg");
    return ESP_ERR_INVALID_ARG;
  }

  // Format the command
  int32_t written = snprintf(buffer,
                             buffer_size,
                             "=%u,\"%s\",%u",
                             write_params->client_id,
                             write_params->host_name,
                             write_params->port);

  // Ensure buffer is in a good state, even if overflow occurred
  if ((written < 0) || ((size_t) written >= buffer_size))
  {
    if (buffer_size > 0U)
    {
      buffer[0] = '\0';
    }
    return ESP_ERR_INVALID_SIZE;
  }

  return ESP_OK;
}

// NOTE: Write command need response parser for this command - NOT JUST OK or ERROR!
static esp_err_t qmtopen_write_parser(const char* response, void* parsed_data)
{
  // Check if we have both OK and the result code
  const char* ok_pos     = strstr(response, "OK");
  const char* result_pos = strstr(response, "+QMTOPEN: ");

  if (!ok_pos || !result_pos)
  {
    ESP_LOGE(TAG, "Missing required response parts");
    return ESP_FAIL;
  }

  // Parse client_id and result
  int client_id, result;
  if (sscanf(result_pos + 10, "%d,%d", &client_id, &result) != 2)
  {
    ESP_LOGE(TAG, "Failed to parse QMTOPEN response");
    return ESP_FAIL;
  }

  // Check result code
  if (result != 0)
  {
    ESP_LOGE(TAG, "QMTOPEN failed with result: %d", result);
    return ESP_FAIL;
  }

  return ESP_OK;
}

// CREG command definition
const at_cmd_t AT_CMD_QMTOPEN = {
    .name        = "QMTOPEN",
    .description = "Open network connection for MQTT client",
    .type_info   = {[AT_CMD_TYPE_TEST]  = {.parser = NULL, .formatter = NULL},
                    [AT_CMD_TYPE_READ]  = {.parser = qmtopen_read_parser, .formatter = NULL},
                    [AT_CMD_TYPE_WRITE] = {.parser    = qmtopen_write_parser,
                                           .formatter = qmtopen_write_formatter}},
    .timeout_ms  = 10000, // Spec says 'depends on network'
    .retry_count = 2};
