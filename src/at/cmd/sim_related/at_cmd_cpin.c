#include "sim_related/at_cmd_cpin.h"

#include "at_cmd_structure.h"

#include <esp_log.h>
#include <string.h> // for strcmp, memset, strstr

static cpin_status_t parse_status_string(const char* status_str)
{
  static const struct
  {
    const char*   str;
    cpin_status_t status;
  } status_map[] = {
      {"READY", CPIN_STATUS_READY},
      {"SIM PIN", CPIN_STATUS_SIM_PIN},
      {"SIM PUK", CPIN_STATUS_SIM_PUK},
      {"SIM PIN2", CPIN_STATUS_SIM_PIN2},
      {"SIM PUK2", CPIN_STATUS_SIM_PUK2},
      {"PH-NET PIN", CPIN_STATUS_PH_NET_PIN},
      {"PH-NET PUK", CPIN_STATUS_PH_NET_PUK},
      {"PH-NETSUB PIN", CPIN_STATUS_PH_NETSUB_PIN},
      {"PH-NETSUB PUK", CPIN_STATUS_PH_NETSUB_PUK},
      {"PH-SP PIN", CPIN_STATUS_PH_SP_PIN},
      {"PH-SP PUK", CPIN_STATUS_PH_SP_PUK},
      {"PH-CORP PIN", CPIN_STATUS_PH_CORP_PIN},
      {"PH-CORP PUK", CPIN_STATUS_PH_CORP_PUK},
  };

  for (size_t i = 0; i < sizeof(status_map) / sizeof(status_map[0]); i++)
  {
    if (strcmp(status_str, status_map[i].str) == 0)
    {
      return status_map[i].status;
    }
  }
  return CPIN_STATUS_UNKNOWN;
}

static esp_err_t cpin_read_parser(const char* response, void* parsed_data)
{
  cpin_read_response_t* read_data      = (cpin_read_response_t*) parsed_data;
  char                  status_str[32] = {0};

  // Initialize response structure
  memset(read_data, 0, sizeof(cpin_read_response_t));

  // Find response start
  const char* start = strstr(response, "+CPIN: ");
  if (!start)
  {
    ESP_LOGW("AT_CMD_CPIN", "No +CPIN: in response");
    return ESP_ERR_INVALID_RESPONSE;
  }

  // Extract status string
  if (sscanf(start + 7, "%31[^\r\n]", status_str) == 1)
  {
    read_data->status       = parse_status_string(status_str);
    read_data->status_valid = true;
    return ESP_OK;
  }

  return ESP_ERR_INVALID_RESPONSE;
}

// Formatter for write command parameters
static esp_err_t cpin_write_formatter(const void* params, char* buffer, size_t buffer_size)
{
  const cpin_write_params_t* write_params = (cpin_write_params_t*) params;

  if (write_params->has_new_pin)
  {
    return snprintf(
        buffer, buffer_size, "=\"%s\",\"%s\"", write_params->pin, write_params->new_pin);
  }
  else
  {
    return snprintf(buffer, buffer_size, "=\"%s\"", write_params->pin);
  }
}

// CPIN command definition
const at_cmd_t AT_CMD_CPIN = {
    .name        = "CPIN",
    .description = "Enter PIN",
    .type_info   = {[AT_CMD_TYPE_TEST]  = {.parser    = NULL, // Test command just returns OK
                                           .formatter = NULL},
                    [AT_CMD_TYPE_READ]  = {.parser = cpin_read_parser, .formatter = NULL},
                    [AT_CMD_TYPE_WRITE] = {.parser    = NULL, // Write command expects OK or ERROR
                                           .formatter = cpin_write_formatter}},
    .timeout_ms  = 5000 // 5 seconds per spec
};
