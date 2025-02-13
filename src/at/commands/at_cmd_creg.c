#include "at_cmd_creg.h"
#include "at_cmd_structure.h"
#include <esp_log.h>
#include <string.h>  // for memset, strstr, strncpy, strlen

static esp_err_t creg_test_parser(const char* response, void* parsed_data) {
    creg_test_response_t* test_data = (creg_test_response_t*)parsed_data;
    memset(test_data, 0, sizeof(creg_test_response_t));

    // Find response start
    const char* start = strstr(response, "+CREG: (");
    if (!start) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    start += 8;  // Skip "+CREG: ("

    // Parse supported modes
    while (*start && test_data->num_modes < 3) {
        if (*start >= '0' && *start <= '2') {
            test_data->supported_modes[test_data->num_modes++] = *start - '0';
        }
        start++;
    }

    return ESP_OK;
}

static esp_err_t creg_read_parser(const char* response, void* parsed_data) {
    creg_read_response_t* read_data = (creg_read_response_t*)parsed_data;
    memset(read_data, 0, sizeof(creg_read_response_t));

    // Find response start
    const char* start = strstr(response, "+CREG: ");
    if (!start) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    start += 7;

    int n, stat;
    char lac[5] = {0}, ci[9] = {0};
    int act = -1;

    // Try to parse with all possible formats
    int matched = sscanf(start, "%d,%d,\"%4[^\"]\",%8[^\"]\",%d",
                        &n, &stat, lac, ci, &act);
    
    if (matched >= 2) {  // At minimum need n and stat
        read_data->n = n;
        read_data->present.has_n = true;
        
        if (stat >= 0 && stat < CREG_STATUS_MAX) {
            read_data->status = (creg_status_t)stat;
            read_data->present.has_status = true;
        }
        
        if (matched >= 3 && strlen(lac) > 0) {
            strncpy(read_data->lac, lac, sizeof(read_data->lac)-1);
            read_data->present.has_lac = true;
        }
        
        if (matched >= 4 && strlen(ci) > 0) {
            strncpy(read_data->ci, ci, sizeof(read_data->ci)-1);
            read_data->present.has_ci = true;
        }
        
        if (matched >= 5 && act >= 0) {
            read_data->act = (creg_act_t)act;
            read_data->present.has_act = true;
        }
        
        return ESP_OK;
    }

    return ESP_ERR_INVALID_RESPONSE;
}

static esp_err_t creg_write_formatter(const void* params, char* buffer, size_t buffer_size) {
    const creg_write_params_t* write_params = (creg_write_params_t*)params;
    if (write_params->n > 2) {  // Validate n range
        return ESP_ERR_INVALID_ARG;
    }
    return snprintf(buffer, buffer_size, "=%d", write_params->n);
}

// CREG command definition
const at_cmd_t AT_CMD_CREG = {
    .name = "CREG",
    .description = "Network Registration Status",
    .type_info = {
        [AT_CMD_TYPE_TEST] = {
            .parser = creg_test_parser,
            .formatter = NULL
        },
        [AT_CMD_TYPE_READ] = {
            .parser = creg_read_parser,
            .formatter = NULL
        },
        [AT_CMD_TYPE_WRITE] = {
            .parser = NULL,
            .formatter = creg_write_formatter
        }
    },
    .timeout_ms = 300,    // 300ms per spec
    .retry_count = 1
};
