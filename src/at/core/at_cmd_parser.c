#include "at_cmd_parser.h"

#include "esp_err.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>

static const char* TAG = "AT CMD PARSER";

// This allows commands that expect a particular data response, to handle ONLY that using their
// parser Checking for basic type command and if there is data to be parsed is handled by this
esp_err_t at_cmd_parse_response(const char* raw_response, at_parsed_response_t* parsed_response)
{
  if (NULL == raw_response || NULL == parsed_response)
  {
    ESP_LOGE(TAG, "Parser arguements invalid");
    return ESP_ERR_INVALID_ARG;
  }

  memset(parsed_response, 0, sizeof(at_parsed_response_t));

  // Handle BASIC response (all commmands will have this)
  if (strstr(raw_response, "OK"))
  {
    parsed_response->has_basic_response   = true;
    parsed_response->basic_response_is_ok = true;
  }
  else if (strstr(raw_response, "ERROR"))
  {
    parsed_response->has_basic_response   = true;
    parsed_response->basic_response_is_ok = false;
  }
  else if (strstr(raw_response, "+CME ERROR:"))
  {
    parsed_response->has_basic_response   = true;
    parsed_response->basic_response_is_ok = false;
    // TODO: Parse CME error code...
  }

  // Look for data response (starts with +)
  const char* data_start = strchr(raw_response, '+');
  if (data_start)
  {
    const char* data_end = strstr(data_start, "\r\n");
    if (data_end)
    {
      parsed_response->has_data_response = true;
      parsed_response->data_response     = (char*) data_start;
      parsed_response->data_response_len = data_end - data_start;
    }
  }

  return ESP_OK;
}

// esp_err_t at_cmd_parse_response(const char*     response,
//                                 size_t          response_len,
//                                 const at_cmd_t* cmd,
//                                 at_cmd_type_t   type,
//                                 void*           parsed_data)
// {
//   if (!response || !cmd || !parsed_data || type >= AT_CMD_TYPE_MAX)
//   {
//     return ESP_ERR_INVALID_ARG;
//   }
//
//   // Check for error responses first
//   if (strstr(response, "ERROR") || strstr(response, "+CME ERROR"))
//   {
//     return ESP_FAIL;
//   }
//
//   // Check if we have a parser for this command type
//   if (!cmd->type_info[type].parser)
//   {
//     ESP_LOGE("AT CMD PARSER", "No Parser found for this cmd");
//     return ESP_ERR_NOT_SUPPORTED;
//   }
//
//   // Call command-specific parser
//   return cmd->type_info[type].parser(response, parsed_data);
// }
