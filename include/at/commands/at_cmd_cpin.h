#pragma once

#include "at_cmd_structure.h"
#include <stdbool.h>

typedef enum {
    CPIN_STATUS_READY = 0,          // Ready, no password needed
    CPIN_STATUS_SIM_PIN = 1,        // SIM PIN required
    CPIN_STATUS_SIM_PUK = 2,        // SIM PUK required
    CPIN_STATUS_SIM_PIN2 = 3,       // SIM PIN2 required
    CPIN_STATUS_SIM_PUK2 = 4,       // SIM PUK2 required
    CPIN_STATUS_PH_NET_PIN = 5,     // Network personalization PIN required
    CPIN_STATUS_PH_NET_PUK = 6,     // Network personalization PUK required
    CPIN_STATUS_PH_NETSUB_PIN = 7,  // Network subset PIN required
    CPIN_STATUS_PH_NETSUB_PUK = 8,  // Network subset PUK required
    CPIN_STATUS_PH_SP_PIN = 9,      // Service provider PIN required
    CPIN_STATUS_PH_SP_PUK = 10,     // Service provider PUK required
    CPIN_STATUS_PH_CORP_PIN = 11,   // Corporate PIN required
    CPIN_STATUS_PH_CORP_PUK = 12,   // Corporate PUK required
    CPIN_STATUS_UNKNOWN = 13        // Unknown status
} cpin_status_t;

// Response structure for read command
typedef struct {
    cpin_status_t status;
    bool status_valid;
} cpin_read_response_t;

// Parameters structure for write command
typedef struct {
    char pin[9];         // PIN/PUK code (8 chars + null terminator)
    char new_pin[9];     // New PIN if PUK is being used
    bool has_new_pin;    // Whether new_pin is being used
} cpin_write_params_t;

// Command declaration
extern const at_cmd_t AT_CMD_CPIN;
