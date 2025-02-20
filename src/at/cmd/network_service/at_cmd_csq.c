#include "at_cmd_csq.h"

#include "at_cmd_structure.h"
#include "enum_utils.h"

#include <esp_log.h>
#include <string.h> // for memset, strstr

static const char* TAG = "AT_CMD_CSQ";

// Define the mapping array for BER values
const enum_str_map_t CSQ_BER_MAP[CSQ_BER_MAP_SIZE] = {
    {CSQ_BER_0, "BER < 0.2%"},
    {CSQ_BER_1, "0.2% <= BER < 0.4%"},
    {CSQ_BER_2, "0.4% <= BER < 0.8%"},
    {CSQ_BER_3, "0.8% <= BER < 1.6%"},
    {CSQ_BER_4, "1.6% <= BER < 3.2%"},
    {CSQ_BER_5, "3.2% <= BER < 6.4%"},
    {CSQ_BER_6, "6.4% <= BER < 12.8%"},
    {CSQ_BER_7, "12.8% <= BER"},
    {CSQ_BER_UNKNOWN, "Unknown or not detectable"}};

// Map RSSI values (0-31, 99) to dBm
int16_t csq_rssi_to_dbm(uint8_t rssi)
{
  if (rssi == CSQ_RSSI_UNKNOWN)
  {
    return 0; // Unknown/invalid
  }

  if (rssi == 0)
  {
    return -113; // -113 dBm or less
  }
  if (rssi == 1)
  {
    return -111; // -111 dBm
  }
  if (rssi >= 2 && rssi <= 30)
  {
    // -109 to -53 dBm, formula: -109 + ((rssi - 2) * 2)
    return -109 + ((rssi - 2) * 2);
  }
  if (rssi == 31)
  {
    return -51; // -51 dBm or greater
  }

  return 0; // Invalid value
}

// Map dBm to RSSI values (0-31, 99)
uint8_t csq_dbm_to_rssi(int16_t dbm)
{
  if (dbm <= -113)
  {
    return 0;
  }
  if (dbm == -111)
  {
    return 1;
  }
  if (dbm >= -109 && dbm <= -53)
  {
    // Convert from dBm to RSSI 2-30 range
    return (uint8_t) (2 + ((dbm + 109) / 2));
  }
  if (dbm >= -51)
  {
    return 31;
  }

  return CSQ_RSSI_UNKNOWN; // Unknown/invalid
}

static esp_err_t csq_cmd_test_type_parser(const char* response, void* parsed_data)
{
  csq_test_response_t* test_data = (csq_test_response_t*) parsed_data;
  if (!test_data || !response)
  {
    return ESP_ERR_INVALID_ARG;
  }

  memset(test_data, 0, sizeof(csq_test_response_t));

  // Find response start
  const char* start = strstr(response, "+CSQ: ");
  if (!start)
  {
    ESP_LOGE(TAG, "Failed to find +CSQ: in response");
    return ESP_ERR_INVALID_RESPONSE;
  }
  start += 6;

  int rssi_min, rssi_max, ber_min, ber_max;
  int rssi_unknown = 0, ber_unknown = 0;

  // Parse response like "(0-31,99),(0-7,99)"
  // First try to parse with unknown values (99)
  int matched = sscanf(start,
                       "(%d-%d,%d),(%d-%d,%d)",
                       &rssi_min,
                       &rssi_max,
                       &rssi_unknown,
                       &ber_min,
                       &ber_max,
                       &ber_unknown);

  if (matched < 4)
  {
    // Try without unknown values
    matched = sscanf(start, "(%d-%d),(%d-%d)", &rssi_min, &rssi_max, &ber_min, &ber_max);

    if (matched < 4)
    {
      ESP_LOGE(TAG, "Failed to parse CSQ test response format");
      return ESP_ERR_INVALID_RESPONSE;
    }
  }

  test_data->rssi_min     = (uint8_t) rssi_min;
  test_data->rssi_max     = (uint8_t) rssi_max;
  test_data->ber_min      = (uint8_t) ber_min;
  test_data->ber_max      = (uint8_t) ber_max;
  test_data->rssi_unknown = (rssi_unknown == 99);
  test_data->ber_unknown  = (ber_unknown == 99);

  ESP_LOGD(TAG,
           "CSQ test parsed: RSSI range %d-%d%s, BER range %d-%d%s",
           test_data->rssi_min,
           test_data->rssi_max,
           test_data->rssi_unknown ? " (with unknown)" : "",
           test_data->ber_min,
           test_data->ber_max,
           test_data->ber_unknown ? " (with unknown)" : "");

  return ESP_OK;
}

static esp_err_t csq_cmd_execute_type_parser(const char* response, void* parsed_data)
{
  csq_execute_response_t* exec_data = (csq_execute_response_t*) parsed_data;
  if (!exec_data || !response)
  {
    return ESP_ERR_INVALID_ARG;
  }

  memset(exec_data, 0, sizeof(csq_execute_response_t));

  // Find response start
  const char* start = strstr(response, "+CSQ: ");
  if (!start)
  {
    ESP_LOGE(TAG, "Failed to find +CSQ: in response");
    return ESP_ERR_INVALID_RESPONSE;
  }
  start += 6;

  int rssi, ber;
  if (sscanf(start, "%d,%d", &rssi, &ber) == 2)
  {
    // Validate ranges
    if ((rssi >= 0 && rssi <= 31) || rssi == 99)
    {
      exec_data->rssi = (uint8_t) rssi;
    }
    else
    {
      ESP_LOGW(TAG, "Invalid RSSI value: %d, defaulting to 99 (unknown)", rssi);
      exec_data->rssi = 99; // Default to unknown for invalid values
    }

    if ((ber >= 0 && ber <= 7) || ber == 99)
    {
      exec_data->ber = (uint8_t) ber;
    }
    else
    {
      ESP_LOGW(TAG, "Invalid BER value: %d, defaulting to 99 (unknown)", ber);
      exec_data->ber = 99; // Default to unknown for invalid values
    }

    ESP_LOGD(TAG, "CSQ parsed: RSSI=%d, BER=%d", exec_data->rssi, exec_data->ber);

    return ESP_OK;
  }

  ESP_LOGE(TAG, "Failed to parse RSSI and BER values");
  return ESP_ERR_INVALID_RESPONSE;
}

// CSQ command definition
const at_cmd_t AT_CMD_CSQ = {
    .name        = "CSQ",
    .description = "Signal Quality Report",
    .type_info   = {[AT_CMD_TYPE_TEST]    = {.parser = csq_cmd_test_type_parser, .formatter = NULL},
                    [AT_CMD_TYPE_READ]    = AT_CMD_TYPE_NOT_IMPLEMENTED,
                    [AT_CMD_TYPE_WRITE]   = AT_CMD_TYPE_NOT_IMPLEMENTED,
                    [AT_CMD_TYPE_EXECUTE] = {.parser    = csq_cmd_execute_type_parser,
                                             .formatter = NULL}},
    .timeout_ms  = 300 // 300ms per spec
};
