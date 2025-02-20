// Open a network connection for an MQTT client
#pragma once
#include "at_cmd_structure.h"

#include <stdint.h>

// Result from write command
typedef enum
{
  QMTOPEN_RESULT_FAILED_TO_OPEN               = -1,
  QMTOPEN_RESULT_OPEN_SUCCESS                 = 0,
  QMTOPEN_RESULT_FAIL_WRONG_PARAMETER         = 1,
  QMTOPEN_RESULT_FAIL_MQTT_CLIENT_ID_OCCUPIED = 2,
  QMTOPEN_RESULT_FAIL_PDP_FAILED_TO_ACTIVATE  = 3,
  QMTOPEN_RESULT_FAIL_PARSE_DOMAIN_NAME_FAIL  = 4,
  QMTOPEN_RESULT_FAIL_NETWORK_CONN_ERROR      = 5
} qmtopen_result_t;

// QMTOPEN read response
typedef struct
{
  int  client_id;
  int  port;
  char host_name[128];
} qmtopen_read_response_t;

// QMTOPEN write parameters
typedef struct
{
  int  client_id;
  int  port;
  char host_name[128];
} qmtopen_write_params_t;

// TODO: QMTOPEN test response
//  typedef struct {
//      uint8_t supported_modes[3];  // Array of supported n values
//      uint8_t num_modes;          // Number of supported modes
//  } creg_test_response_t;

// Command declaration
extern const at_cmd_t AT_CMD_QMTOPEN;
