// Close a network connection for an MQTT client
#pragma once
#include "at_cmd_structure.h"
#include "enum_utils.h"

#include <stdint.h>

// Result from close network connection operation
typedef enum
{
  QMTCLOSE_RESULT_FAILED_TO_CLOSE = -1, // Failed to close network
  QMTCLOSE_RESULT_CLOSE_SUCCESS   = 0   // Network closed successfully
} qmtclose_result_t;

#define QMTCLOSE_RESULT_MAP_SIZE 2
extern const enum_str_map_t QMTCLOSE_RESULT_MAP[QMTCLOSE_RESULT_MAP_SIZE];

#define QMTCLOSE_CLIENT_IDX_MIN 0
#define QMTCLOSE_CLIENT_IDX_MAX 5

// Present flags structure for responses
typedef struct
{
  bool has_client_idx : 1;
  bool has_result : 1;
} qmtclose_present_flags_t;

// QMTCLOSE test response - for range of supported client indexes
typedef struct
{
  uint8_t client_idx_min; // Minimum client index
  uint8_t client_idx_max; // Maximum client index
} qmtclose_test_response_t;

// QMTCLOSE write parameters - for closing a network connection
typedef struct
{
  uint8_t client_idx; // MQTT client identifier (0-5)
} qmtclose_write_params_t;

// QMTCLOSE write response - async response after closing a connection
typedef struct
{
  uint8_t                  client_idx; // MQTT client identifier (0-5)
  qmtclose_result_t        result;     // Command execution result
  qmtclose_present_flags_t present;    // Flags for which fields are present
} qmtclose_write_response_t;

// Command declaration
extern const at_cmd_t AT_CMD_QMTCLOSE;
