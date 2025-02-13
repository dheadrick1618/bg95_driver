#include "at_cmd_parser.h"

#include "esp_log.h"

#include <stdio.h>
#include <string.h>

esp_err_t at_cmd_parse_response(const char*     response,
                                size_t          response_len,
                                const at_cmd_t* cmd,
                                at_cmd_type_t   type,
                                void*           parsed_data)
{
  if (!response || !cmd || !parsed_data || type >= AT_CMD_TYPE_MAX)
  {
    return ESP_ERR_INVALID_ARG;
  }

  // Check for error responses first
  if (strstr(response, "ERROR") || strstr(response, "+CME ERROR"))
  {
    return ESP_FAIL;
  }

  // Check if we have a parser for this command type
  if (!cmd->type_info[type].parser)
  {
    ESP_LOGE("AT CMD PARSER", "No Parser found for this cmd");
    return ESP_ERR_NOT_SUPPORTED;
  }

  // Call command-specific parser
  return cmd->type_info[type].parser(response, parsed_data);
}
