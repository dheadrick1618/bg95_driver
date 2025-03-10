
#include "at_cmd_cfun.h"

#include "at_cmd_structure.h"
#include "esp_err.h"
#include "esp_log.h"

#include <string.h>

static const char* TAG = "AT_CMD_CFUN";

const enum_str_map_t CFUN_FUN_TYPE_MAP[CFUN_FUN_TYPE_MAP_SIZE] = {
    {CFUN_FUN_TYPE_MINIMUM, "Minimum Functionality"},
    {CFUN_FUN_TYPE_FULL, "Fulll Functionality"},
    {CFUN_FUN_TYPE_DISABLE_TX_AND_RX, "Disable RF Tx and Rx Functionality"},
};

static esp_err_t cfun_read_parser(const char* response, void* parsed_data)
{
  /* Validate input parameters */
  if (NULL == response || NULL == parsed_data)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  cfun_read_response_t* read_params = (cfun_read_response_t*) parsed_data;

  (void) memset(read_params, 0, sizeof(cfun_read_response_t));

  const char* start = strstr(response, "+CFUN: ");
  if (!start)
  {
    return ESP_ERR_INVALID_RESPONSE;
  }
  start += 7; // Skip  "+CFUN: "

  int fun_type;
  if (sscanf(start, "%d", &fun_type) == 1)
  {
    read_params->fun_type = fun_type;
    return ESP_OK;
  }

  return ESP_ERR_INVALID_RESPONSE;
}

static esp_err_t cfun_write_formatter(const void* params, char* buffer, size_t buffer_size)
{

  if (!params || !buffer || buffer_size == 0)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  const cfun_write_params_t* write_params = (const cfun_write_params_t*) params;
  int                        written      = 0;

  written = snprintf(buffer, buffer_size, "=%d", write_params->fun_type);

  if (write_params->fun_type == 3 || write_params->fun_type > CFUN_FUN_TYPE_DISABLE_TX_AND_RX)
  {
    return ESP_ERR_INVALID_ARG;
  }

  // Ensure buffer always in good state, even if overflow
  if ((written < 0) || ((size_t) written >= buffer_size))
  {
    /* Ensure null termination in case of overflow */
    if (buffer_size > 0U)
    {
      buffer[0] = '\0';
    }
    return ESP_ERR_INVALID_SIZE;
  }
  return ESP_OK;
}

const at_cmd_t AT_CMD_CFUN = {
    .name        = "CFUN",
    .description = "Set UE Functionality",
    .type_info   = {[AT_CMD_TYPE_TEST]    = AT_CMD_TYPE_NOT_IMPLEMENTED,
                    [AT_CMD_TYPE_READ]    = {.parser = cfun_read_parser, .formatter = NULL},
                    [AT_CMD_TYPE_WRITE]   = {.parser = NULL, .formatter = cfun_write_formatter},
                    [AT_CMD_TYPE_EXECUTE] = AT_CMD_TYPE_DOES_NOT_EXIST},
    .timeout_ms  = 5000 // NOTE: Spec says 15 s , but this is too long for us to wait
};
