#pragma once

#include <esp_err.h>
#include <stdbool.h>
#include <stddef.h> //For size_t
#include <stdint.h> //For uint32_t

// #define AT_PREFIX "AT+"
// #define AT_SUFFIX "\r\n"

// #define TEST_CMD(cmd) AT_PREFIX cmd "=?" AT_SUFFIX
// #define READ_CMD(cmd) AT_PREFIX cmd "?" AT_SUFFIX
// #define WRITE_CMD(cmd) AT_PREFIX cmd "=" // No suffix - params will be added
// #define EXECUTE_CMD(cmd) AT_PREFIX cmd AT_SUFFIX
//------

// NOTE: Macro for explicitly defining AT command types that do NOT exist or are NOT implemented
#define AT_CMD_TYPE_NOT_IMPLEMENTED {.parser = NULL, .formatter = NULL}
#define AT_CMD_TYPE_DOES_NOT_EXIST {.parser = NULL, .formatter = NULL}

// #define AT_CMD_TYPE_NOT_IMPLEMENTED {.parser = NULL, .formatter = NULL, .response_type =
// AT_CMD_RESPONSE_TYPE_SIMPLE_ONLY} #define AT_CMD_TYPE_DOES_NOT_EXIST {.parser = NULL, .formatter
// = NULL, .response_type = AT_CMD_RESPONSE_TYPE_SIMPLE_ONLY}

#define AT_CMD_MAX_RESPONSE_LEN 2048
#define AT_CMD_MAX_CMD_LEN 256

typedef enum
{
  AT_CMD_TYPE_TEST    = 0U, // AT+CMD=?
  AT_CMD_TYPE_READ    = 1U, // AT+CMD?
  AT_CMD_TYPE_WRITE   = 2U, // AT+CMD=<params>
  AT_CMD_TYPE_EXECUTE = 3U, // AT+CMD
  AT_CMD_TYPE_MAX     = 4U
} at_cmd_type_t;

typedef enum
{
  AT_CMD_RESPONSE_TYPE_SIMPLE_ONLY,   // Only expects OK/ERROR (no data response)
  AT_CMD_RESPONSE_TYPE_DATA_REQUIRED, // Requires a data response (+CMD: ...)
  AT_CMD_RESPONSE_TYPE_DATA_OPTIONAL, // Data response is optional
} at_cmd_response_type_t;

// Generic command response definition
typedef struct
{
  char*         raw_response;
  size_t        response_len;
  void*         parsed_data; // Type-specific struct holding result of parsing response data
  at_cmd_type_t cmd_type;
  esp_err_t     status;
} at_cmd_response_t;

// fxn pointer for generic parameter parsing
typedef esp_err_t (*at_param_parser_t)(const char* response, void* parsed_data);

// fxn pointer for generic parameter formatting
typedef esp_err_t (*at_param_formatter_t)(const void* params, char* buffer, size_t buffer_size);

typedef struct
{
  at_param_parser_t      parser;        // Function to parse parameters
  at_param_formatter_t   formatter;     // Function to format parameters
  at_cmd_response_type_t response_type; // Expected response type
} at_cmd_type_info_t;

typedef struct
{
  const char*        name;
  const char*        description;
  at_cmd_type_info_t type_info[AT_CMD_TYPE_MAX];
  uint32_t           timeout_ms;
} at_cmd_t;

static inline bool at_cmd_type_is_implemented(const at_cmd_t* cmd, at_cmd_type_t type)
{
  if (!cmd || type >= AT_CMD_TYPE_MAX)
  {
    return false;
  }
  return (cmd->type_info[type].parser != NULL || cmd->type_info[type].formatter != NULL);
}
