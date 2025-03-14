// Open a network connection for an MQTT client
#pragma once
#include "at_cmd_structure.h"
#include "enum_utils.h"

#include <stdint.h>

// Result from open network connection operation
typedef enum
{
  QMTOPEN_RESULT_FAILED_TO_OPEN      = -1, // Failed to open network
  QMTOPEN_RESULT_OPEN_SUCCESS        = 0,  // Network opened successfully
  QMTOPEN_RESULT_WRONG_PARAMETER     = 1,  // Wrong parameter
  QMTOPEN_RESULT_MQTT_ID_OCCUPIED    = 2,  // MQTT client identifier is occupied
  QMTOPEN_RESULT_FAILED_ACTIVATE_PDP = 3,  // Failed to activate PDP
  QMTOPEN_RESULT_FAILED_PARSE_DOMAIN = 4,  // Failed to parse domain name
  QMTOPEN_RESULT_NETWORK_CONN_ERROR  = 5   // Network connection error
} qmtopen_result_t;

#define QMTOPEN_RESULT_MAP_SIZE 7
extern const enum_str_map_t QMTOPEN_RESULT_MAP[QMTOPEN_RESULT_MAP_SIZE];

#define QMTOPEN_CLIENT_IDX_MIN 0
#define QMTOPEN_CLIENT_IDX_MAX 5
#define QMTOPEN_HOST_NAME_MAX_SIZE 100
#define QMTOPEN_PORT_MIN 0
#define QMTOPEN_PORT_MAX 65535

// Present flags structure for responses
typedef struct
{
  bool has_client_idx : 1;
  bool has_host_name : 1;
  bool has_port : 1;
  bool has_result : 1;
} qmtopen_present_flags_t;

// QMTOPEN read response - for querying current connection status
typedef struct
{
  uint8_t  client_idx;                                // MQTT client identifier (0-5)
  char     host_name[QMTOPEN_HOST_NAME_MAX_SIZE + 1]; // Server address (IP or domain name)
  uint16_t port;                                      // Server port (0-65535)
  qmtopen_present_flags_t present;                    // Flags for which fields are present
} qmtopen_read_response_t;

// QMTOPEN write parameters - for opening a network connection
typedef struct
{
  uint8_t  client_idx;                                // MQTT client identifier (0-5)
  char     host_name[QMTOPEN_HOST_NAME_MAX_SIZE + 1]; // Server address (IP or domain name)
  uint16_t port;                                      // Server port (0-65535)
} qmtopen_write_params_t;

// QMTOPEN write response - async response after opening a connection
typedef struct
{
  uint8_t                 client_idx; // MQTT client identifier (0-5)
  qmtopen_result_t        result;     // Command execution result
  qmtopen_present_flags_t present;    // Flags for which fields are present
} qmtopen_write_response_t;

// Command declaration
extern const at_cmd_t AT_CMD_QMTOPEN;
