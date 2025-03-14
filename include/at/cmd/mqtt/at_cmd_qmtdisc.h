// at_cmd_qmtdisc.h
#pragma once
#include "at_cmd_structure.h"
#include "enum_utils.h"

#include <stdint.h>

// Result from MQTT disconnect operation
typedef enum
{
  QMTDISC_RESULT_SUCCESS        = 0, // Disconnect message sent successfully
  QMTDISC_RESULT_FAILED_TO_SEND = 2  // Failed to send disconnect message
} qmtdisc_result_t;

#define QMTDISC_RESULT_MAP_SIZE 2
extern const enum_str_map_t QMTDISC_RESULT_MAP[QMTDISC_RESULT_MAP_SIZE];

#define QMTDISC_CLIENT_IDX_MIN 0
#define QMTDISC_CLIENT_IDX_MAX 5

// Present flags structure for responses
typedef struct
{
  bool has_client_idx : 1;
  bool has_result : 1;
} qmtdisc_present_flags_t;

// QMTDISC test response - for range of supported client indexes
typedef struct
{
  uint8_t client_idx_min; // Minimum client index
  uint8_t client_idx_max; // Maximum client index
} qmtdisc_test_response_t;

// QMTDISC write parameters - for disconnecting from MQTT server
typedef struct
{
  uint8_t client_idx; // MQTT client identifier (0-5)
} qmtdisc_write_params_t;

// QMTDISC write response - async response after disconnect attempt
typedef struct
{
  uint8_t                 client_idx; // MQTT client identifier (0-5)
  qmtdisc_result_t        result;     // Command execution result
  qmtdisc_present_flags_t present;    // Flags for which fields are present
} qmtdisc_write_response_t;

// Command declaration
extern const at_cmd_t AT_CMD_QMTDISC;
