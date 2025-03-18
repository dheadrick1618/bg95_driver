#include "at_cmd_at.h"

static const char* TAG = "AT_CMD_AT";

// Dummy formatter for the AT command
// We don't need to do anything special, but we need this to pass the implementation check
static esp_err_t at_execute_formatter(const void* params, char* buffer, size_t buffer_size)
{
  // The AT command has no parameters, so we just return success
  // The actual formatting is handled by the general formatter in at_cmd_formatter.c
  return ESP_OK;
}

// AT command definition
const at_cmd_t AT_CMD_AT = {
    .name        = "AT", // This is a basic AT command without the + prefix
    .description = "Basic AT Command",
    .type_info   = {[AT_CMD_TYPE_TEST]    = AT_CMD_TYPE_DOES_NOT_EXIST,
                    [AT_CMD_TYPE_READ]    = AT_CMD_TYPE_DOES_NOT_EXIST,
                    [AT_CMD_TYPE_WRITE]   = AT_CMD_TYPE_DOES_NOT_EXIST,
                    [AT_CMD_TYPE_EXECUTE] = {.parser        = NULL,
                                             .formatter     = at_execute_formatter,
                                             .response_type = AT_CMD_RESPONSE_TYPE_SIMPLE_ONLY}},
    .timeout_ms  = 300 // 300ms should be enough for basic AT command
};
