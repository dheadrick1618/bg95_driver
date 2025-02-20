
#include "enum_utils.h"

#include <string.h> //For strcmp

const char* enum_to_str(int value, const enum_str_map_t* mapping, size_t map_size)
{
  if (NULL == mapping)
  {
    return "INVALID_MAP";
  }

  for (size_t i = 0U; i < map_size; i++)
  {
    if (value == mapping[i].value)
    {
      return mapping[i].string;
    }
  }

  return "UNKNOWN";
}

enum_convert_result_t str_to_enum(const char* str, const enum_str_map_t* mapping, size_t map_size)
{
  enum_convert_result_t result = {.value = -1, .is_valid = false, .error = ESP_ERR_INVALID_ARG};

  if ((NULL == str) || (NULL == mapping) || (0U == map_size))
  {
    return result;
  }

  for (size_t i = 0U; i < map_size; i++)
  {
    if (0 == strcmp(str, mapping[i].string))
    {
      result.value    = mapping[i].value;
      result.is_valid = true;
      result.error    = ESP_OK;
      break;
    }
  }

  return result;
}
