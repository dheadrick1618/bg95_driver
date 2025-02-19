// at_cmd_handler.c
#include "at_cmd_handler.h"

#include "at_cmd_formatter.h"
#include "at_cmd_parser.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/projdefs.h"

#include <string.h>

static const char* TAG = "AT_CMD_HANDLER";

esp_err_t at_cmd_handler_init(at_cmd_handler_t* handler, bg95_uart_interface_t* uart)
{
  if (!handler || !uart)
  {
    return ESP_ERR_INVALID_ARG;
  }

  memset(handler, 0, sizeof(at_cmd_handler_t));
  handler->uart = *uart;
  return ESP_OK;
}

bool has_command_terminated(const char* raw_response, const at_cmd_t* cmd, at_cmd_type_t type)
{
  if (NULL == raw_response || NULL == cmd)
  {
    ESP_LOGE(TAG, "Invalid args provided to 'has_cmd_terminated' helper fxn");
    return false;
  }

  if (type >= AT_CMD_TYPE_MAX || type < 0)
  {
    ESP_LOGE(TAG, "Invalid AT CMD TYPE arg provided to 'has_cmd_termianted' helper fxn");
    return false;
  }

  // First check if we have a basic response terminator
  const char* ok_pos    = strstr(raw_response, AT_OK);
  const char* error_pos = strstr(raw_response, AT_ERROR);
  const char* cme_pos   = strstr(raw_response, AT_CME_ERROR);

  // No terminator found yet
  if (!ok_pos && !error_pos && !cme_pos)
  {
    return false;
  }

  // If it's an error response, just need CRLF after it
  if (error_pos || cme_pos)
  {
    const char* marker = error_pos ? error_pos : cme_pos;
    return strstr(marker, AT_CRLF) != NULL;
  }

  // We have OK - if command type doesn't expect data response, just need CRLF after OK
  if (!cmd->type_info[type].parser)
  {
    return strstr(ok_pos, AT_CRLF) != NULL;
  }

  // Command expects data response - need both OK and data response with CRLF
  const char* data_pos = strstr(raw_response, cmd->name);
  if (!data_pos)
  {
    return false; // No data response found yet
  }

  // Make sure both OK and data response end with CRLF
  return strstr(ok_pos, AT_CRLF) && strstr(data_pos, AT_CRLF);
}

esp_err_t at_cmd_handler_send_and_receive_cmd(at_cmd_handler_t* handler,
                                              const at_cmd_t*   cmd,
                                              at_cmd_type_t     type,
                                              const void*       params,
                                              void*             response_data)
{
  if (!handler || !cmd)
  {
    ESP_LOGE(TAG, "Invalid arguments provided");
    return ESP_ERR_INVALID_ARG;
  }

  if (!handler->uart.write || !handler->uart.read || !handler->uart.context)
  {
    ESP_LOGE(TAG, "UART interface not properly initialized");
    return ESP_ERR_INVALID_STATE;
  }

  // Format command
  char      cmd_str[AT_CMD_MAX_CMD_LEN];
  esp_err_t err = format_at_cmd(cmd, type, params, cmd_str, sizeof(cmd_str));
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "AT cmd formatting failed for %s", cmd->name);
    return err;
  }

  // Send command
  ESP_LOGI(TAG, "Sending command: %s (timeout: %lu ms)", cmd_str, (long unsigned) cmd->timeout_ms);
  err = handler->uart.write(cmd_str, strlen(cmd_str), handler->uart.context);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "UART interface WRITE failed: %s", esp_err_to_name(err));
    return err;
  }

  // Allocate buffer for reading response
  char* raw_response = malloc(AT_CMD_MAX_RESPONSE_LEN);
  if (!raw_response)
  {
    return ESP_ERR_NO_MEM;
  }

  // Read response in chunks until complete or timeout
  uint32_t start_time       = pdTICKS_TO_MS(xTaskGetTickCount());
  size_t   total_bytes_read = 0;
  char     temp_buffer[AT_CMD_READ_CHUNK_SIZE];
  bool     response_complete = false;

  while ((pdTICKS_TO_MS(xTaskGetTickCount()) - start_time) < cmd->timeout_ms)
  {
    size_t bytes_read = 0;

    err = handler->uart.read(temp_buffer,
                             sizeof(temp_buffer) - 1,
                             &bytes_read,
                             AT_CMD_READ_CHUNK_INTERVAL_MS,
                             handler->uart.context);

    if (err == ESP_OK && bytes_read > 0)
    {
      // Check for buffer overflow
      if ((total_bytes_read + bytes_read) >= AT_CMD_MAX_RESPONSE_LEN)
      {
        ESP_LOGE(TAG, "UART interface READ failed - buffer overflow");
        free(raw_response);
        return ESP_ERR_INVALID_SIZE;
      }

      // Append new data to raw response buffer, using total_bytes_read as index
      // NOTE: memcpy does not check for null terminating char - copies exactly 'bytes read' amount
      memcpy(raw_response + total_bytes_read, temp_buffer, bytes_read);
      total_bytes_read += bytes_read;
      raw_response[total_bytes_read] = '\0';

      if (has_command_terminated(raw_response, cmd, type))
      {
        response_complete = true;
        break;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(1));
  }

  if (!response_complete)
  {
    ESP_LOGE(TAG, "TIMEOUT waiting for response. This was recv'd: %s", raw_response);
    free(raw_response);
    return ESP_ERR_TIMEOUT;
  }

  ESP_LOGI(TAG, "Received response: %s", raw_response);

  // Parse base response first
  at_parsed_response_t parsed_base = {0};
  err                              = at_cmd_parse_response(raw_response, &parsed_base);
  if (err != ESP_OK)
  {
    free(raw_response);
    return err;
  }

  // Check if response indicates an error
  if (!parsed_base.basic_response_is_ok)
  {
    ESP_LOGE(TAG, "Parsed basic response is ERROR (not OK)");
    free(raw_response);
    return ESP_FAIL;
    // return parsed_base.cme_error_code ? ESP_ERR_AT_CME_ERROR : ESP_ERR_AT_ERROR;
  }

  // If command has a data parser defined, response data pointer is provided, has_data_response flag
  // is set - then attempt to parse the data response using the the specific parsing fxn provided
  // via fxn pointer
  if (cmd->type_info[type].parser && response_data && parsed_base.has_data_response)
  {
    err = cmd->type_info[type].parser(raw_response, response_data);
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Command-specific Data response parsing failed");
    }
  }

  free(raw_response);
  return err;
}
