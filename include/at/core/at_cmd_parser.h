#pragma once

#include "at_cmd_structure.h"

esp_err_t parse_at_cmd_response(
    const char* response, 
    size_t response_len, 
    const at_cmd_t* cmd,
    at_cmd_type_t type, 
    void* parsed_data
);
