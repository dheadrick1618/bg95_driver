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

esp_err_t validate_basic_response(const char* raw_response, at_parsed_response_t* parsed_base)
{
  esp_err_t err = at_cmd_parse_response(raw_response, parsed_base);
  if (err != ESP_OK)
  {
    return err;
  }

  if (!parsed_base->basic_response_is_ok)
  {
    ESP_LOGE(TAG, "Parsed basic response is ERROR (not OK)");
    return ESP_FAIL;
  }

  return ESP_OK;
}

esp_err_t parse_at_cmd_specific_data_response(const at_cmd_t*             cmd,
                                              at_cmd_type_t               type,
                                              const char*                 raw_response,
                                              const at_parsed_response_t* parsed_base,
                                              void*                       response_data)
{
  if (cmd->type_info[type].parser && response_data && parsed_base->has_data_response == true)
  {
    return cmd->type_info[type].parser(raw_response, response_data);
  }
  return ESP_OK;
}

esp_err_t read_at_cmd_response(at_cmd_handler_t* handler,
                               const at_cmd_t*   cmd,
                               at_cmd_type_t     type,
                               char*             response_buffer,
                               size_t            buffer_size)
{
  uint32_t start_time       = pdTICKS_TO_MS(xTaskGetTickCount());
  size_t   total_bytes_read = 0;
  char     temp_buffer[AT_CMD_READ_CHUNK_SIZE];
  bool     response_complete = false;

  while ((pdTICKS_TO_MS(xTaskGetTickCount()) - start_time) < cmd->timeout_ms)
  {
    size_t    bytes_read = 0;
    esp_err_t err        = handler->uart.read(temp_buffer,
                                       sizeof(temp_buffer) - 1,
                                       &bytes_read,
                                       AT_CMD_READ_CHUNK_INTERVAL_MS,
                                       handler->uart.context);

    if (err == ESP_OK && bytes_read > 0)
    {
      if ((total_bytes_read + bytes_read) >= buffer_size)
      {
        return ESP_ERR_INVALID_SIZE;
      }

      memcpy(response_buffer + total_bytes_read, temp_buffer, bytes_read);
      total_bytes_read += bytes_read;
      response_buffer[total_bytes_read] = '\0';

      if (has_command_terminated(response_buffer, cmd, type))
      {
        response_complete = true;
        break;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  return response_complete ? ESP_OK : ESP_ERR_TIMEOUT;
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

  // Validate UART interface
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

  // Allocate and read response
  char* raw_response = malloc(AT_CMD_MAX_RESPONSE_LEN);
  if (!raw_response)
  {
    return ESP_ERR_NO_MEM;
  }

  err = read_at_cmd_response(handler, cmd, type, raw_response, AT_CMD_MAX_RESPONSE_LEN);
  if (err != ESP_OK)
  {
    free(raw_response);
    return err;
  }

  ESP_LOGI(TAG, "Received response: %s", raw_response);

  // Parse and validate basic response
  at_parsed_response_t parsed_base = {0};
  err                              = validate_basic_response(raw_response, &parsed_base);
  if (err != ESP_OK)
  {
    free(raw_response);
    return err;
  }

  // Parse command-specific response if needed
  err = parse_at_cmd_specific_data_response(cmd, type, raw_response, &parsed_base, response_data);

  free(raw_response);
  return err;
}
