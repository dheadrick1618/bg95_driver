#pragma once

#include <esp_err.h>
#include <stdbool.h>

typedef struct
{
  int         value;
  const char* string;
} enum_str_map_t;

typedef struct
{
  int       value;
  bool      is_valid;
  esp_err_t error;
} enum_convert_result_t;

// Generic conversion function declarations
const char*           enum_to_str(int value, const enum_str_map_t* mapping, size_t map_size);
enum_convert_result_t str_to_enum(const char* str, const enum_str_map_t* mapping, size_t map_size);
