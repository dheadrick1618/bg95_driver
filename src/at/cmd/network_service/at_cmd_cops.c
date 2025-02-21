#include "at_cmd_cops.h"

#include "at_cmd_structure.h"

#include <esp_log.h>
#include <string.h> // for memset, strstr, strncpy

static const char* TAG = "AT_CMD_COPS";

// Define enum-to-string mappings
const enum_str_map_t COPS_STAT_MAP[COPS_STAT_MAP_SIZE] = {
    {COPS_STAT_UNKNOWN, "Unknown"},
    {COPS_STAT_OPERATOR_AVAILABLE, "Operator available"},
    {COPS_STAT_CURRENT_OPERATOR, "Current operator"},
    {COPS_STAT_OPERATOR_FORBIDDEN, "Operator forbidden"}};

const enum_str_map_t COPS_MODE_MAP[COPS_MODE_MAP_SIZE] = {
    {COPS_MODE_AUTO, "Automatic mode"},
    {COPS_MODE_MANUAL, "Manual operator selection"},
    {COPS_MODE_DEREGISTER, "Manual deregister from network"},
    {COPS_MODE_SET_FORMAT, "Set only format"},
    {COPS_MODE_MANUAL_AUTO, "Manual/automatic selection"}};

const enum_str_map_t COPS_FORMAT_MAP[COPS_FORMAT_MAP_SIZE] = {
    {COPS_FORMAT_LONG_ALPHA, "Long format alphanumeric"},
    {COPS_FORMAT_SHORT_ALPHA, "Short format alphanumeric"},
    {COPS_FORMAT_NUMERIC, "Numeric"}};

const enum_str_map_t COPS_ACT_MAP[COPS_ACT_MAP_SIZE] = {
    {COPS_ACT_GSM, "GSM"}, {COPS_ACT_EMTC, "eMTC"}, {COPS_ACT_NB_IOT, "NB-IoT"}};

static esp_err_t cops_read_parser(const char* response, void* parsed_data)
{
  if (!response || !parsed_data)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  cops_read_response_t* read_data = (cops_read_response_t*) parsed_data;
  memset(read_data, 0, sizeof(cops_read_response_t));

  // Find response start
  const char* start = strstr(response, "+COPS: ");
  if (!start)
  {
    ESP_LOGE(TAG, "Failed to find +COPS: in response");
    return ESP_ERR_INVALID_RESPONSE;
  }
  start += 7; // Skip "+COPS: "

  int  mode, format, act;
  char operator_name[64] = {0};

  // Try to parse with different possible formats
  // First try with all fields
  int matched = sscanf(start, "%d,%d,\"%63[^\"]\",%d", &mode, &format, operator_name, &act);

  // Mode is always present
  if (matched >= 1)
  {
    if (mode >= 0 && mode <= 4)
    {
      read_data->mode             = (cops_mode_t) mode;
      read_data->present.has_mode = true;

      // Format and operator
      if (matched >= 2)
      {
        if (format >= 0 && format <= 2)
        {
          read_data->format             = (cops_format_t) format;
          read_data->present.has_format = true;
        }

        if (matched >= 3 && operator_name[0] != '\0')
        {
          strcpy(read_data->operator_name, operator_name);
          read_data->present.has_operator = true;
        }

        // Act field
        if (matched >= 4)
        {
          // Validate act values
          if (act == 0 || act == 8 || act == 9)
          {
            read_data->act             = (cops_act_t) act;
            read_data->present.has_act = true;
          }
        }
      }

      return ESP_OK;
    }
  }

  ESP_LOGE(TAG, "Failed to parse COPS read response");
  return ESP_ERR_INVALID_RESPONSE;
}

// static esp_err_t cops_write_formatter(const void* params, char* buffer, size_t buffer_size)
// {
//   if (!params || !buffer || buffer_size == 0)
//   {
//     ESP_LOGE(TAG, "Invalid arguments");
//     return ESP_ERR_INVALID_ARG;
//   }
//
//   const cops_write_params_t* write_params = (const cops_write_params_t*) params;
//   int                        written      = 0;
//
//   // Validate parameters
//   if (write_params->mode > COPS_MODE_MANUAL_AUTO)
//   {
//     ESP_LOGE(TAG, "Invalid mode parameter: %d", write_params->mode);
//     return ESP_ERR_INVALID_ARG;
//   }
//
//   // Format based on mode
//   if (write_params->mode == COPS_MODE_AUTO || write_params->mode == COPS_MODE_DEREGISTER)
//   {
//     // For auto or deregister, only mode is needed
//     written = snprintf(buffer, buffer_size, "=%d", write_params->mode);
//   }
//   else if (write_params->mode == COPS_MODE_SET_FORMAT)
//   {
//     // For set format, need mode and format
//     if (write_params->format > COPS_FORMAT_NUMERIC)
//     {
//       ESP_LOGE(TAG, "Invalid format parameter: %d", write_params->format);
//       return ESP_ERR_INVALID_ARG;
//     }
//     written = snprintf(buffer, buffer_size, "=%d,%d", write_params->mode, write_params->format);
//   }
//   else
//   {
//     // For manual or manual/auto, need all parameters
//     if (write_params->format > COPS_FORMAT_NUMERIC)
//     {
//       ESP_LOGE(TAG, "Invalid format parameter: %d", write_params->format);
//       return ESP_ERR_INVALID_ARG;
//     }
//
//     // Act is optional for manual mode
//     if (write_params->present.has_act)
//     {
//       written = snprintf(buffer,
//                          buffer_size,
//                          "=%d,%d,\"%s\",%d",
//                          write_params->mode,
//                          write_params->format,
//                          write_params->operator_name,
//                          write_params->act);
//     }
//     else
//     {
//       written = snprintf(buffer,
//                          buffer_size,
//                          "=%d,%d,\"%s\"",
//                          write_params->mode,
//                          write_params->format,
//                          write_params->operator_name);
//     }
//   }
//
//   // Check if buffer was too small
//   if (written < 0 || (size_t) written >= buffer_size)
//   {
//     ESP_LOGE(TAG, "Buffer too small for formatting COPS write command");
//     if (buffer_size > 0)
//     {
//       buffer[0] = '\0'; // Ensure null-terminated in case of overflow
//     }
//     return ESP_ERR_INVALID_SIZE;
//   }
//
//   return ESP_OK;
// }

// COPS command definition
const at_cmd_t AT_CMD_COPS = {
    .name        = "COPS",
    .description = "Operator Selection",
    .type_info   = {[AT_CMD_TYPE_TEST]    = AT_CMD_TYPE_NOT_IMPLEMENTED, // TODO: this
                    [AT_CMD_TYPE_READ]    = {.parser = cops_read_parser, .formatter = NULL},
                    [AT_CMD_TYPE_WRITE]   = AT_CMD_TYPE_NOT_IMPLEMENTED, // TODO: this
                    [AT_CMD_TYPE_EXECUTE] = AT_CMD_TYPE_NOT_IMPLEMENTED},
    .timeout_ms  = 10000 // 180 seconds is spec
};
