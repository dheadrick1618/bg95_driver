#include "at_cmd_cops.h"

#include "at_cmd_structure.h"

#include <esp_log.h>
#include <string.h> // for memset, strstr, strncpy

static esp_err_t cops_read_parser(const char* response, void* parsed_data)
{
  cops_read_response_t* read_data = (cops_read_response_t*) parsed_data;
  memset(read_data, 0, sizeof(cops_read_response_t));

  // Find response start
  const char* start = strstr(response, "+COPS: ");
  if (!start)
  {
    return ESP_ERR_INVALID_RESPONSE;
  }
  start += 7;

  int  mode, format, act;
  char operator_name[64];

  // Parse with all fields optional except mode
  int matched = sscanf(start, "%d,%d,\"%63[^\"]\",%d", &mode, &format, operator_name, &act);

  if (matched >= 1)
  {
    read_data->mode             = (cops_mode_t) mode;
    read_data->present.has_mode = true;

    if (matched >= 2)
    {
      read_data->format             = (cops_format_t) format;
      read_data->present.has_format = true;
    }

    if (matched >= 3)
    {
      strncpy(read_data->operator_name, operator_name, sizeof(read_data->operator_name) - 1);
      read_data->present.has_operator = true;
    }

    if (matched >= 4)
    {
      read_data->act             = (cops_act_t) act;
      read_data->present.has_act = true;
    }

    return ESP_OK;
  }

  return ESP_ERR_INVALID_RESPONSE;
}

// Parser functions for COPS command
static esp_err_t cops_test_parser(const char* response, void* parsed_data)
{
  cops_test_response_t* test_data = (cops_test_response_t*) parsed_data;
  memset(test_data, 0, sizeof(cops_test_response_t));

  // Find response start
  const char* start = strstr(response, "+COPS: ");
  if (!start)
  {
    return ESP_ERR_INVALID_RESPONSE;
  }
  start += 7; // Skip "+COPS: "

  // Parse each operator in parentheses
  const char* pos = start;
  while (*pos && test_data->num_operators < 10)
  {
    if (*pos++ != '(')
      continue;

    cops_operator_info_t* op = &test_data->operators[test_data->num_operators];
    int                   stat;
    char                  name[64];
    int                   act;

    if (sscanf(pos, "%d,\"%63[^\"]\",%*[^,],%*[^,],%d", &stat, name, &act) >= 2)
    {
      op->stat = (cops_stat_t) stat;
      strncpy(op->operator_name, name, sizeof(op->operator_name) - 1);
      op->act                  = (cops_act_t) act;
      op->present.has_stat     = true;
      op->present.has_operator = true;
      op->present.has_act      = true;
      test_data->num_operators++;
    }

    // Find next operator entry
    pos = strchr(pos, ')');
    if (!pos)
      break;
  }

  return ESP_OK;
}

static esp_err_t cops_write_formatter(const void* params, char* buffer, size_t buffer_size)
{
  const cops_write_params_t* cops_params = (cops_write_params_t*) params;
  return snprintf(buffer,
                  buffer_size,
                  "%d,%d,\"%s\",%d",
                  cops_params->mode,
                  cops_params->format,
                  cops_params->operator_name,
                  cops_params->act);
}

// COPS command definition
const at_cmd_t AT_CMD_COPS = {
    .name        = "COPS",
    .description = "Operator Selection",
    .type_info   = {[AT_CMD_TYPE_TEST] =
                        {
                            .parser    = cops_test_parser,
                            .formatter = NULL // No params for TEST
                      },
                    [AT_CMD_TYPE_READ]  = {.parser = cops_read_parser, .formatter = NULL},
                    [AT_CMD_TYPE_WRITE] = {.parser = NULL, // Write command just expects OK or ERROR
                                           .formatter = cops_write_formatter}},
    .timeout_ms  = 180000, // 180 seconds per spec
};
