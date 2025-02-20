#include "at_cmd_csq.h"

#include "at_cmd_structure.h"

#include <esp_log.h>
#include <string.h> // for memset, strstr

int16_t csq_rssi_to_dbm(uint8_t rssi)
{
  if (rssi == 99)
  {
    return 0; // Unknown/invalid
  }

  if (rssi == 0)
  {
    return -113;
  }
  if (rssi == 1)
  {
    return -111;
  }
  if (rssi >= 2 && rssi <= 30)
  {
    // -109 to -53 dBm
    return -109 + ((rssi - 2) * 2);
  }
  if (rssi == 31)
  {
    return -51;
  }

  return 0; // Invalid value
}

static esp_err_t csq_test_parser(const char* response, void* parsed_data)
{
  csq_test_response_t* test_data = (csq_test_response_t*) parsed_data;
  memset(test_data, 0, sizeof(csq_test_response_t));

  // Find response start
  const char* start = strstr(response, "+CSQ: ");
  if (!start)
  {
    return ESP_ERR_INVALID_RESPONSE;
  }
  start += 6;

  int rssi_min, rssi_max, ber_min, ber_max;

  // Parse response like "(0-31,99),(0-7,99)"
  if (sscanf(start, "(%d-%d,%*d),(%d-%d,%*d)", &rssi_min, &rssi_max, &ber_min, &ber_max) == 4)
  {
    test_data->rssi_min     = rssi_min;
    test_data->rssi_max     = rssi_max;
    test_data->ber_min      = ber_min;
    test_data->ber_max      = ber_max;
    test_data->rssi_unknown = true; // 99 is always supported
    test_data->ber_unknown  = true; // 99 is always supported
    return ESP_OK;
  }

  return ESP_ERR_INVALID_RESPONSE;
}

static esp_err_t csq_execute_parser(const char* response, void* parsed_data)
{
  csq_response_t* exec_data = (csq_response_t*) parsed_data;
  memset(exec_data, 0, sizeof(csq_response_t));

  // Find response start
  const char* start = strstr(response, "+CSQ: ");
  if (!start)
  {
    return ESP_ERR_INVALID_RESPONSE;
  }
  start += 6;

  int rssi, ber;
  if (sscanf(start, "%d,%d", &rssi, &ber) == 2)
  {
    exec_data->rssi             = rssi;
    exec_data->ber              = ber;
    exec_data->present.has_rssi = true;
    exec_data->present.has_ber  = true;

    // Convert RSSI to dBm
    exec_data->rssi_dbm = csq_rssi_to_dbm(rssi);

    return ESP_OK;
  }

  return ESP_ERR_INVALID_RESPONSE;
}

// CSQ command definition
const at_cmd_t AT_CMD_CSQ = {
    .name        = "CSQ",
    .description = "Signal Quality Report",
    .type_info   = {[AT_CMD_TYPE_TEST]    = {.parser = csq_test_parser, .formatter = NULL},
                    [AT_CMD_TYPE_EXECUTE] = {.parser = csq_execute_parser, .formatter = NULL}},
    .timeout_ms  = 300, // 300ms per spec
};
