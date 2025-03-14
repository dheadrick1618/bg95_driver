#include "at_cmd_cpin.h"

#include "at_cmd_structure.h"
#include "enum_utils.h"

#include <esp_log.h>
#include <string.h> // for strcmp, memset, strstr

static const char* TAG = "CPIN_AT_CMD";

// Define the mapping array
const enum_str_map_t CPIN_STATUS_MAP[] = {{CPIN_STATUS_READY, "READY"},
                                          {CPIN_STATUS_SIM_PIN, "SIM PIN"},
                                          {CPIN_STATUS_SIM_PUK, "SIM PUK"},
                                          {CPIN_STATUS_SIM_PIN2, "SIM PIN2"},
                                          {CPIN_STATUS_SIM_PUK2, "SIM PUK2"},
                                          {CPIN_STATUS_PH_NET_PIN, "PH-NET PIN"},
                                          {CPIN_STATUS_PH_NET_PUK, "PH-NET PUK"},
                                          {CPIN_STATUS_PH_NETSUB_PIN, "PH-NETSUB PIN"},
                                          {CPIN_STATUS_PH_NETSUB_PUK, "PH-NETSUB PUK"},
                                          {CPIN_STATUS_PH_SP_PIN, "PH-SP PIN"},
                                          {CPIN_STATUS_PH_SP_PUK, "PH-SP PUK"},
                                          {CPIN_STATUS_PH_CORP_PIN, "PH-CORP PIN"},
                                          {CPIN_STATUS_PH_CORP_PUK, "PH-CORP PUK"},
                                          {CPIN_STATUS_UNKNOWN, "UNKNOWN"}};

static esp_err_t cpin_cmd_read_type_parser(const char* response, void* parsed_data)
{
  cpin_read_response_t* read_data      = (cpin_read_response_t*) parsed_data;
  char                  status_str[32] = {0};

  // Initialize response structure
  memset(read_data, 0, sizeof(cpin_read_response_t));

  // Find response start
  const char* start = strstr(response, "+CPIN: ");
  if (!start)
  {
    ESP_LOGW(TAG, "Start index search failed. No +CPIN: in response");
    return ESP_ERR_INVALID_RESPONSE;
  }

  // Extract status string
  if (sscanf(start + 7, "%31[^\r\n]", status_str) == 1)
  {
    enum_convert_result_t res = str_to_enum(
        status_str, CPIN_STATUS_MAP, sizeof(CPIN_STATUS_MAP) / sizeof(CPIN_STATUS_MAP[0]));

    if (res.is_valid)
    {
      read_data->status       = (cpin_status_t) res.value;
      read_data->status_valid = true;
      return ESP_OK;
    }
  }

  return ESP_ERR_INVALID_RESPONSE;
}

// Formatter for write command outgoing parameters
static esp_err_t cpin_cmd_write_type_formatter(const void* params, char* buffer, size_t buffer_size)
{
  if (!params || !buffer || buffer_size == 0)
  {
    return ESP_ERR_INVALID_ARG;
  }

  const cpin_write_params_t* write_params = (cpin_write_params_t*) params;
  int                        written;

  if (write_params->has_new_pin)
  {
    written =
        snprintf(buffer, buffer_size, "=\"%s\",\"%s\"", write_params->pin, write_params->new_pin);
  }
  else
  {
    written = snprintf(buffer, buffer_size, "=\"%s\"", write_params->pin);
  }

  // Check if buffer was too small or other error occurred
  if (written < 0 || (size_t) written >= buffer_size)
  {
    if (buffer_size > 0)
    {
      buffer[0] = '\0'; // Ensure buffer is null-terminated
    }
    return ESP_ERR_INVALID_SIZE;
  }

  return ESP_OK;
}

// CPIN command definition
const at_cmd_t AT_CMD_CPIN = {
    .name        = "CPIN",
    .description = "Enter PIN",
    .type_info   = {[AT_CMD_TYPE_EXECUTE] = AT_CMD_TYPE_NOT_IMPLEMENTED,
                    [AT_CMD_TYPE_TEST]    = {.parser        = NULL,
                                             .formatter     = NULL,
                                             .response_type = AT_CMD_RESPONSE_TYPE_SIMPLE_ONLY},
                    [AT_CMD_TYPE_READ]    = {.parser        = cpin_cmd_read_type_parser,
                                             .formatter     = NULL,
                                             .response_type = AT_CMD_RESPONSE_TYPE_DATA_REQUIRED},
                    [AT_CMD_TYPE_WRITE] =
                        {.parser    = NULL,
                         .formatter = cpin_cmd_write_type_formatter,
                         .response_type =
                             AT_CMD_RESPONSE_TYPE_DATA_REQUIRED}}, // Expects Basic Response only
    .timeout_ms  = 5000                                            // 5 seconds per spec
};
