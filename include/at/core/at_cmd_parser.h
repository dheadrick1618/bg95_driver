#pragma once

#include "esp_err.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct
{
  bool has_basic_response;   // Whether we've gotten OK/ERROR/CME
  bool basic_response_is_ok; // Was it OK vs ERROR/CME
  int  cme_error_code;       // If CME error, the code

  bool   has_data_response; // Whether we got a data response
  char*  data_response;     // Points to start of data response if exists
  size_t data_response_len; // Length of data response
} at_parsed_response_t;

esp_err_t at_cmd_parse_response(const char* raw_response, at_parsed_response_t* parsed);

// OLD APPROACH
//  esp_err_t parse_at_cmd_response(const char*     response,
//                                  size_t          response_len,
//                                  const at_cmd_t* cmd,
//                                  at_cmd_type_t   type,
//                                  void*           parsed_data);
