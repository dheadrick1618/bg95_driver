// src/at/cmd/network_service/at_cmd_qcsq.c
#include "at_cmd_qcsq.h"

#include "at_cmd_structure.h"
#include "esp_log.h"

#include <string.h>

static const char* TAG = "AT_CMD_QCSQ";

// Define the mapping arrays
const enum_str_map_t QCSQ_SYSMODE_MAP[QCSQ_SYSMODE_MAP_SIZE] = {
    {QCSQ_SYSMODE_NOSERVICE, "NOSERVICE"},
    {QCSQ_SYSMODE_GSM, "GSM"},
    {QCSQ_SYSMODE_EMTC, "eMTC"},
    {QCSQ_SYSMODE_NBIOT, "NBIoT"}};

// Helper functions for signal quality conversion
int16_t qcsq_rssi_to_dbm(int16_t rssi)
{
  // RSSI is already in dBm in QCSQ response
  return rssi;
}

int16_t qcsq_rsrp_to_dbm(int16_t rsrp)
{
  // RSRP is already in dBm in QCSQ response
  return rsrp;
}

float qcsq_sinr_to_db(int16_t sinr)
{
  // SINR is in 1/5th of a dB (0-250 maps to -20dB to +30dB)
  return (float) sinr / 5.0f - 20.0f;
}

float qcsq_rsrq_to_db(int16_t rsrq)
{
  // RSRQ is already in dB in QCSQ response
  return (float) rsrq;
}

static esp_err_t qcsq_test_parser(const char* response, void* parsed_data)
{
  if (NULL == response || NULL == parsed_data)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  qcsq_test_response_t* test_data = (qcsq_test_response_t*) parsed_data;

  // Initialize the test response structure
  memset(test_data, 0, sizeof(qcsq_test_response_t));

  // Find response start
  const char* start = strstr(response, "+QCSQ: ");
  if (!start)
  {
    ESP_LOGE(TAG, "Failed to find +QCSQ: in response");
    return ESP_ERR_INVALID_RESPONSE;
  }
  start += 7; // Skip "+QCSQ: "

  // Parse supported system modes
  if (strstr(start, "\"NOSERVICE\""))
    test_data->supports_sysmode[QCSQ_SYSMODE_NOSERVICE] = true;
  if (strstr(start, "\"GSM\""))
    test_data->supports_sysmode[QCSQ_SYSMODE_GSM] = true;
  if (strstr(start, "\"eMTC\""))
    test_data->supports_sysmode[QCSQ_SYSMODE_EMTC] = true;
  if (strstr(start, "\"NBIoT\""))
    test_data->supports_sysmode[QCSQ_SYSMODE_NBIOT] = true;

  return ESP_OK;
}

static esp_err_t qcsq_execute_parser(const char* response, void* parsed_data)
{
  if (NULL == response || NULL == parsed_data)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  qcsq_execute_response_t* exec_data = (qcsq_execute_response_t*) parsed_data;

  // Initialize output structure
  memset(exec_data, 0, sizeof(qcsq_execute_response_t));

  // Find response start
  const char* start = strstr(response, "+QCSQ: ");
  if (!start)
  {
    ESP_LOGE(TAG, "Failed to find +QCSQ: in response");
    return ESP_ERR_INVALID_RESPONSE;
  }
  start += 7; // Skip "+QCSQ: "

  // Parse system mode - it's a quoted string
  char sysmode_str[16] = {0};
  if (sscanf(start, "\"%15[^\"]\"", sysmode_str) != 1)
  {
    ESP_LOGE(TAG, "Failed to parse system mode");
    return ESP_ERR_INVALID_RESPONSE;
  }

  // Set system mode
  bool found_sysmode = false;
  for (int i = 0; i < QCSQ_SYSMODE_MAP_SIZE; i++)
  {
    if (strcmp(sysmode_str, QCSQ_SYSMODE_MAP[i].string) == 0)
    {
      exec_data->sysmode             = (qcsq_sysmode_t) QCSQ_SYSMODE_MAP[i].value;
      exec_data->present.has_sysmode = true;
      found_sysmode                  = true;
      break;
    }
  }

  if (!found_sysmode)
  {
    ESP_LOGE(TAG, "Unknown system mode: %s", sysmode_str);
    return ESP_ERR_INVALID_RESPONSE;
  }

  // Skip to after the sysmode and the following quote and comma
  const char* value_start = strchr(start, '\"');
  if (!value_start)
    return ESP_ERR_INVALID_RESPONSE;
  value_start++; // Skip first quote
  value_start = strchr(value_start, '\"');
  if (!value_start)
    return ESP_ERR_INVALID_RESPONSE;
  value_start++; // Skip second quote

  // Skip comma if present
  if (*value_start == ',')
    value_start++;

  // Depending on the system mode, parse the appropriate number of values
  int value1 = 0, value2 = 0, value3 = 0, value4 = 0;
  int parsed_values = 0;

  switch (exec_data->sysmode)
  {
    case QCSQ_SYSMODE_NOSERVICE:
      // No values to parse
      break;

    case QCSQ_SYSMODE_GSM:
      // Only one value (GSM_RSSI)
      parsed_values = sscanf(value_start, "%d", &value1);
      if (parsed_values >= 1)
      {
        exec_data->value1             = (int16_t) value1;
        exec_data->present.has_value1 = true;
      }
      break;

    case QCSQ_SYSMODE_EMTC:
    case QCSQ_SYSMODE_NBIOT:
      // Four values (LTE_RSSI, LTE_RSRP, LTE_SINR, LTE_RSRQ)
      parsed_values = sscanf(value_start, "%d,%d,%d,%d", &value1, &value2, &value3, &value4);

      if (parsed_values >= 1)
      {
        exec_data->value1             = (int16_t) value1;
        exec_data->present.has_value1 = true;
      }
      if (parsed_values >= 2)
      {
        exec_data->value2             = (int16_t) value2;
        exec_data->present.has_value2 = true;
      }
      if (parsed_values >= 3)
      {
        exec_data->value3             = (int16_t) value3;
        exec_data->present.has_value3 = true;
      }
      if (parsed_values >= 4)
      {
        exec_data->value4             = (int16_t) value4;
        exec_data->present.has_value4 = true;
      }
      break;
  }

  ESP_LOGI(TAG, "QCSQ parsed: sysmode=%s", sysmode_str);
  if (exec_data->present.has_value1)
    ESP_LOGI(TAG, "  value1=%d (RSSI)", exec_data->value1);
  if (exec_data->present.has_value2)
    ESP_LOGI(TAG, "  value2=%d (RSRP)", exec_data->value2);
  if (exec_data->present.has_value3)
    ESP_LOGI(TAG, "  value3=%d (SINR)", exec_data->value3);
  if (exec_data->present.has_value4)
    ESP_LOGI(TAG, "  value4=%d (RSRQ)", exec_data->value4);

  return ESP_OK;
}

// QCSQ command definition
const at_cmd_t AT_CMD_QCSQ = {
    .name        = "QCSQ",
    .description = "Query and Report Signal Strength",
    .type_info   = {[AT_CMD_TYPE_TEST]    = {.parser        = qcsq_test_parser,
                                             .formatter     = NULL,
                                             .response_type = AT_CMD_RESPONSE_TYPE_DATA_REQUIRED},
                    [AT_CMD_TYPE_READ]    = AT_CMD_TYPE_DOES_NOT_EXIST,
                    [AT_CMD_TYPE_WRITE]   = AT_CMD_TYPE_DOES_NOT_EXIST,
                    [AT_CMD_TYPE_EXECUTE] = {.parser        = qcsq_execute_parser,
                                             .formatter     = NULL,
                                             .response_type = AT_CMD_RESPONSE_TYPE_DATA_REQUIRED}},
    .timeout_ms  = 300 // 300ms per spec
};
