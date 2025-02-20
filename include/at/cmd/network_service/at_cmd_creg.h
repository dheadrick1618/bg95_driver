#pragma once

#include "at_cmd_structure.h"
#include <stdbool.h>
#include <stdint.h> //for uint types

//TODO: Implement API fxn for setting this 
typedef enum 
{
    CREG_NET_REG_MODE_DISABLE_UNSOLICITED_RESULT_CODE = 0U,
    CREG_NET_REG_MODE_ENABLE_UNSOLICITED_RESULT_CODE = 1U,
    CREG_NET_REG_MODE_ENABLE_UNSOLICITED_RESULT_CODE_AND_LOCATION = 2U
} creg_net_reg_mode_t;

typedef enum {
    CREG_STATUS_NOT_SEARCHING = 0,     // Not registered, not searching
    CREG_STATUS_HOME = 1,              // Registered to home network
    CREG_STATUS_SEARCHING = 2,         // Not registered, searching
    CREG_STATUS_DENIED = 3,            // Registration denied
    CREG_STATUS_UNKNOWN = 4,           // Unknown
    CREG_STATUS_ROAMING = 5,           // Registered, roaming
    CREG_STATUS_MAX = 6
} creg_status_t;

// Network access technology
typedef enum {
    CREG_ACT_GSM = 0,
    CREG_ACT_EMTC = 8,
    CREG_ACT_NBIOT = 9,
    CREG_ACT_MAX = 10
} creg_act_t;

// CREG read response
typedef struct {
    uint8_t n;                 // Registration code mode
    creg_status_t status;      // Registration status
    char lac[5];              // Location Area Code (hex string)
    char ci[9];               // Cell ID (hex string)
    creg_act_t act;           // Access Technology
    struct {
        bool has_n: 1;
        bool has_status: 1;
        bool has_lac: 1;
        bool has_ci: 1;
        bool has_act: 1;
    } present;
} creg_read_response_t;

// CREG write parameters
typedef struct {
    uint8_t n;    // Registration code mode (0-2)
} creg_write_params_t;

// CREG test response
typedef struct {
    uint8_t supported_modes[3];  // Array of supported n values
    uint8_t num_modes;          // Number of supported modes
} creg_test_response_t;

// Command declaration
extern const at_cmd_t AT_CMD_CREG;
