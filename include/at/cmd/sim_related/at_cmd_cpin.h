#pragma once

#include "at_cmd_structure.h"
#include "enum_utils.h"

#include <stdbool.h>

#define CPIN_STATUS_MAP_SIZE 14 // number of enum values for map array size

typedef enum
{
  CPIN_STATUS_READY         = 0U,  // Ready, no password needed
  CPIN_STATUS_SIM_PIN       = 1U,  // SIM PIN required
  CPIN_STATUS_SIM_PUK       = 2U,  // SIM PUK required
  CPIN_STATUS_SIM_PIN2      = 3U,  // SIM PIN2 required
  CPIN_STATUS_SIM_PUK2      = 4U,  // SIM PUK2 required
  CPIN_STATUS_PH_NET_PIN    = 5U,  // Network personalization PIN required
  CPIN_STATUS_PH_NET_PUK    = 6U,  // Network personalization PUK required
  CPIN_STATUS_PH_NETSUB_PIN = 7U,  // Network subset PIN required
  CPIN_STATUS_PH_NETSUB_PUK = 8U,  // Network subset PUK required
  CPIN_STATUS_PH_SP_PIN     = 9U,  // Service provider PIN required
  CPIN_STATUS_PH_SP_PUK     = 10U, // Service provider PUK required
  CPIN_STATUS_PH_CORP_PIN   = 11U, // Corporate PIN required
  CPIN_STATUS_PH_CORP_PUK   = 12U, // Corporate PUK required
  CPIN_STATUS_UNKNOWN       = 13U  // Unknown status
} cpin_status_t;
extern const enum_str_map_t CPIN_STATUS_MAP[CPIN_STATUS_MAP_SIZE];

// Response structure for read command
typedef struct
{
  cpin_status_t status;
  bool          status_valid;
} cpin_read_response_t;

// Parameters structure for write command
typedef struct
{
  char pin[9];      // PIN/PUK code (8 chars + null terminator)
  char new_pin[9];  // New PIN if PUK is being used
  bool has_new_pin; // Whether new_pin is being used
} cpin_write_params_t;

// Command declaration
extern const at_cmd_t AT_CMD_CPIN;
