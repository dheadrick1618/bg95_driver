// include/at/cmd/network_service/at_cmd_qcsq.h
#pragma once

#include "at_cmd_structure.h"
#include "enum_utils.h"

#include <stdbool.h>
#include <stdint.h>

// System mode definitions
typedef enum
{
  QCSQ_SYSMODE_NOSERVICE = 0,
  QCSQ_SYSMODE_GSM       = 1,
  QCSQ_SYSMODE_EMTC      = 2,
  QCSQ_SYSMODE_NBIOT     = 3
} qcsq_sysmode_t;

#define QCSQ_SYSMODE_MAP_SIZE 4
extern const enum_str_map_t QCSQ_SYSMODE_MAP[QCSQ_SYSMODE_MAP_SIZE];

// Present flags structure for responses
typedef struct
{
  bool has_sysmode : 1;
  bool has_value1 : 1; // GSM_RSSI or LTE_RSSI
  bool has_value2 : 1; // LTE_RSRP
  bool has_value3 : 1; // LTE_SINR
  bool has_value4 : 1; // LTE_RSRQ
} qcsq_present_flags_t;

// Test command response structure
typedef struct
{
  bool supports_sysmode[QCSQ_SYSMODE_MAP_SIZE];
} qcsq_test_response_t;

// Execute command response structure
typedef struct
{
  qcsq_sysmode_t       sysmode; // System mode
  int16_t              value1;  // GSM_RSSI or LTE_RSSI
  int16_t              value2;  // LTE_RSRP
  int16_t              value3;  // LTE_SINR
  int16_t              value4;  // LTE_RSRQ
  qcsq_present_flags_t present; // Flags for which fields are present
} qcsq_execute_response_t;

// Helper functions for signal quality conversion (similar to CSQ but adapted for QCSQ)
int16_t qcsq_rssi_to_dbm(int16_t rssi);
int16_t qcsq_rsrp_to_dbm(int16_t rsrp);
float   qcsq_sinr_to_db(int16_t sinr);
float   qcsq_rsrq_to_db(int16_t rsrq);

// Command declaration
extern const at_cmd_t AT_CMD_QCSQ;
