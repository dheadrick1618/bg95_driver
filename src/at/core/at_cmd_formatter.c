#include "at_cmd_formatter.h"

#include "at_cmd_structure.h"
#include "esp_err.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>

static esp_err_t format_test_type_command(const char* cmd_name, char* buffer, size_t buffer_size)
{
  int written = snprintf(buffer, buffer_size, "AT+%s=?\r\n", cmd_name);
  if (written < 0 || written >= buffer_size)
  {
    return ESP_FAIL;
  }
  return ESP_OK;
}

static esp_err_t format_read_type_command(const char* cmd_name, char* buffer, size_t buffer_size)
{
  int written = snprintf(buffer, buffer_size, "AT+%s?\r\n", cmd_name);
  if (written < 0 || written >= buffer_size)
  {
    return ESP_FAIL;
  }
  return ESP_OK;
}

static esp_err_t format_execute_type_command(const char* cmd_name, char* buffer, size_t buffer_size)
{
  int written;

  // Check if this is the basic AT command or a regular AT+ command
  if (strcmp(cmd_name, "AT") == 0)
  {
    written = snprintf(buffer, buffer_size, "AT\r\n");
  }
  else
  {
    written = snprintf(buffer, buffer_size, "AT+%s\r\n", cmd_name);
  }

  if (written < 0 || written >= buffer_size)
  {
    return ESP_FAIL;
  }

  return ESP_OK;
}

static esp_err_t format_write_type_command(const char*          cmd_name,
                                           const void*          params,
                                           at_param_formatter_t formatter,
                                           char*                buffer,
                                           size_t               buffer_size)
{
  int written = snprintf(buffer, buffer_size, "AT+%s", cmd_name);
  if (written < 0 || written >= buffer_size)
  {
    return ESP_FAIL;
  }

  // Format parameters - but don't overwrite what we've written
  char      param_buffer[256] = {0};
  esp_err_t err               = formatter(params, param_buffer, sizeof(param_buffer));
  if (err != ESP_OK)
  {
    return err;
  }

  // Append parameters (which already include the '=')
  int params_written = snprintf(buffer + written, buffer_size - written, "%s", param_buffer);
  if (params_written < 0 || params_written >= (buffer_size - written))
  {
    return ESP_FAIL;
  }
  written += params_written;

  // Add terminator
  int terminator_len = snprintf(buffer + written, buffer_size - written, "\r\n");
  if (terminator_len < 0 || terminator_len >= (buffer_size - written))
  {
    return ESP_FAIL;
  }

  return ESP_OK;
}

esp_err_t format_at_cmd(
    const at_cmd_t* cmd, at_cmd_type_t type, const void* params, char* buffer, size_t buffer_size)
{
  switch (type)
  {
    case AT_CMD_TYPE_TEST:
      return format_test_type_command(cmd->name, buffer, buffer_size);

    case AT_CMD_TYPE_READ:
      return format_read_type_command(cmd->name, buffer, buffer_size);

    case AT_CMD_TYPE_EXECUTE:
      return format_execute_type_command(cmd->name, buffer, buffer_size);

    case AT_CMD_TYPE_WRITE:
      if (!cmd->type_info[type].formatter)
      {
        // NOTE: Every write type command  must have an associted formatter fxn
        ESP_LOGE("AT CMD FORMATTER", "No formatter specified for a write type of this command");
        return ESP_ERR_INVALID_STATE;
      }
      return format_write_type_command(
          cmd->name, params, cmd->type_info[type].formatter, buffer, buffer_size);

    default:
      return ESP_ERR_INVALID_ARG;
  }
}
