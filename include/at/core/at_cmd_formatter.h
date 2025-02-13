#pragma once 

#include "at_cmd_structure.h"

esp_err_t format_at_cmd(
    const at_cmd_t *cmd,
    at_cmd_type_t type,
    const void* params,
    char* output_buffer,
    size_t buffer_size
);
