#include "at_cmd_handler.h"

#include "at_cmd_formatter.h"
#include "at_cmd_structure.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/projdefs.h"

#include <inttypes.h> // for format specifiers for esp log
#include <string.h>   //for strlen fxn

static const char* TAG = "AT_CMD_HANDLER";

static bool is_response_complete(const char* response, at_cmd_type_t type)
{
  // First check for error condition in the response string
  if (strstr(response, AT_ERROR) || strstr(response, AT_CME_ERROR))
  {
    return true;
  }

  // Look for OK terminator
  const char* ok_pos = strstr(response, AT_OK);
  if (!ok_pos)
  {
    return false;
  }

  // For READ/TEST commands, ensure we have the response data before OK
  if (type == AT_CMD_TYPE_READ || type == AT_CMD_TYPE_TEST)
  {
    // Response should start with CR/LF
    if (strncmp(response, AT_CRLF, strlen(AT_CRLF)) != 0)
    {
      return false;
    }

    // Look for the '+' prefix after initial CR/LF
    const char* plus_pos = strchr(response + strlen(AT_CRLF), '+');
    if (!plus_pos || plus_pos > ok_pos)
    {
      return false;
    }

    // Ensure there's at least one set of CR/LF between data and OK
    const char* data_end = strstr(plus_pos, AT_CRLF);
    if (!data_end || data_end > ok_pos)
    {
      return false;
    }
  }
  return true;
}

esp_err_t at_cmd_handler_send_and_receive_cmd(at_cmd_handler_t*  handler,
                                              const at_cmd_t*    cmd,
                                              at_cmd_type_t      type,
                                              const void*        params,
                                              at_cmd_response_t* response)
{
  if (!handler || !cmd || !response)
  {
    return ESP_ERR_INVALID_ARG;
  }
  if (!handler->uart.write || !handler->uart.read || !handler->uart.context)
  {
    ESP_LOGE(TAG, "UART interface not properly initialized - AT cmd send and recv FAILED");
    return ESP_ERR_INVALID_STATE;
  }

  ESP_LOGI(TAG, "Formatting command");

  // Format command
  char      cmd_str[AT_CMD_MAX_CMD_LEN];
  esp_err_t err = format_at_cmd(cmd, type, params, cmd_str, sizeof(cmd_str));
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "cmd formatting for %s FAILED", cmd->name);
    return err;
  }
  else
  {
    ESP_LOGI(TAG, "Formatted cmd: %s", cmd_str);
  }

  // Allocate response buffer
  response->raw_response = malloc(AT_CMD_MAX_RESPONSE_LEN);
  if (!response->raw_response)
  {
    return ESP_ERR_NO_MEM;
  }
  response->response_len = 0;
  response->cmd_type     = type;
  response->status       = ESP_OK;

  // Send command
  ESP_LOGI(TAG, "\nSending cmd:\n timeout: %" PRIu32 "ms\n cmd str: %s", cmd->timeout_ms, cmd_str);
  err = handler->uart.write(cmd_str, strlen(cmd_str), handler->uart.context);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Write failed: %s", esp_err_to_name(err));
    free(response->raw_response);
    return err;
  }

  ESP_LOGI(TAG, "Waiting for response...");

  // Read response in chunks, with timeout

  uint32_t start_time       = pdTICKS_TO_MS(xTaskGetTickCount());
  size_t   total_bytes_read = 0;
  char     temp_buffer[AT_CMD_READ_CHUNK_SIZE];
  bool     response_complete = false;

  while ((pdTICKS_TO_MS(xTaskGetTickCount()) - start_time) < cmd->timeout_ms)
  {
    size_t bytes_read_in_this_chunk = 0;

    // Read chunk from UART buffer with short readtime
    err = handler->uart.read(temp_buffer,
                             sizeof(temp_buffer) - 1,
                             &bytes_read_in_this_chunk,
                             AT_CMD_READ_CHUNK_INTERVAL_MS,
                             handler->uart.context);

    // If read is successful and bytes read read
    if (err == ESP_OK && bytes_read_in_this_chunk > 0)
    {
      // TODO: Check to be sure  we  don't overflow the  buffer
      if ((total_bytes_read + bytes_read_in_this_chunk) >= AT_CMD_MAX_RESPONSE_LEN)
      {
        free(response->raw_response);
        return ESP_FAIL;
      }

      // Append data read from this chunk to the whole 'raw_response' buffer
      memcpy(response->raw_response + total_bytes_read, temp_buffer, bytes_read_in_this_chunk);
      // Increase total bytes read count
      total_bytes_read += bytes_read_in_this_chunk;
      // null terminate the response buffer
      response->raw_response[total_bytes_read] = '\0';
      response->response_len                   = total_bytes_read;

      if (is_response_complete(response->raw_response, type))
      {
        response_complete = true;
        break;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(1));
  }

  if (!response_complete)
  {
    free(response->raw_response);
    return ESP_ERR_TIMEOUT;
  }

  ESP_LOGI(TAG, "Received response raw (with added null term): %s", response->raw_response);

  //  Parse response if parser exists for this command type
  // The (void ptr) parsed_data attribute of the response struct will be cast and modified to hold
  // the struct that contains the expected response data from that command type
  if (cmd->type_info[type].parser)
  {
    err              = cmd->type_info[type].parser(response->raw_response, response->parsed_data);
    response->status = err;
  }

  return ESP_OK;
}
