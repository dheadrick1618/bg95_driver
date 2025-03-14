
#include "at_cmd_cgatt.h"

#include "esp_err.h"

#include <string.h> //for strstr

// static esp_err_t cgatt_test_parser(const char* response, void* parsed_data) {
//
// }

static esp_err_t cgatt_read_parser(const char* response, void* parsed_data)
{

  if ((NULL == response) || NULL == parsed_data)
  {
    return ESP_ERR_INVALID_ARG;
  }

  cgatt_read_params_t* const read_params = (cgatt_read_params_t*) parsed_data;

  // Find response start
  const char* start = strstr(response, "+CGATT: ");
  if (!start)
  {
    return ESP_ERR_INVALID_RESPONSE;
  }
  start += 8; // Skip  "+CGATT: "

  int state;
  if (sscanf(start, "%d", &state) == 1)
  {
    read_params->state = state;
    return ESP_OK;
  }

  return ESP_ERR_INVALID_RESPONSE;
}

static esp_err_t cgatt_write_formatter(const void* params, char* buffer, size_t buffer_size)
{
  /* Validate parameters */
  if ((NULL == params) || (NULL == buffer) || (0U == buffer_size))
  {
    return ESP_ERR_INVALID_ARG;
  }

  // TODO: Validate param range

  const cgatt_write_params_t* const write_params = (const cgatt_write_params_t*) params;

  // Validate range of provided params
  if (write_params->state >= CGATT_STATE_MAX)
  {
    return ESP_ERR_INVALID_ARG;
  }

  int32_t written = snprintf(buffer, buffer_size, "=%d", write_params->state);

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

// CGATT command definition
const at_cmd_t AT_CMD_CGATT = {
    .name        = "CGATT",
    .description = "PS Attach or Detach",
    .type_info =
        {// [AT_CMD_TYPE_TEST] = {
         //     .parser = cgatt_test_parser,
         //     .formatter = NULL
         // },
         [AT_CMD_TYPE_READ]  = {.parser = cgatt_read_parser, .formatter = NULL},
         [AT_CMD_TYPE_WRITE] = {.parser = NULL, .formatter = cgatt_write_formatter}},
    .timeout_ms = 140000, // 140 s per spec
};
