#pragma once

#include "at_cmd_structure.h"
#include "enum_utils.h"

#include <stdbool.h>
#include <stdint.h>

// Test type command response structure for supported ranges
typedef struct
{
  uint8_t rssi_min;     // Minimum supported RSSI value
  uint8_t rssi_max;     // Maximum supported RSSI value
  uint8_t ber_min;      // Minimum supported BER value
  uint8_t ber_max;      // Maximum supported BER value
  bool    rssi_unknown; // Whether 99 (unknown) is supported for RSSI
  bool    ber_unknown;  // Whether 99 (unknown) is supported for BER
} csq_test_response_t;

// Execute type command response structure with RSSI and BER values
typedef struct
{
  uint8_t rssi; // Raw RSSI value (0-31, 99 for unknown)
  uint8_t ber;  // Bit Error Rate (0-7, 99 for unknown)
} csq_execute_response_t;

// RSSI values and their dBm equivalents
typedef enum
{
  CSQ_RSSI_MIN = 0,      // -113 dBm or less
                         // Values 0-31 represent the range -113 to -51 dBm
  CSQ_RSSI_MAX     = 31, // -51 dBm or greater
  CSQ_RSSI_UNKNOWN = 99  // Not known or not detectable
} csq_rssi_value_t;
// #define CSQ_RSSI_MAP_SIZE 34 // 0-31 plus UNKNOWN(99) plus MIN/MAX for convenience
// NOTE: Generic enum-str converstion not implemented yet for enum values with ranges

// BER values according to 3GPP TS 45.008
typedef enum
{
  CSQ_BER_0       = 0, // BER < 0.2%
  CSQ_BER_1       = 1, // 0.2% <= BER < 0.4%
  CSQ_BER_2       = 2, // 0.4% <= BER < 0.8%
  CSQ_BER_3       = 3, // 0.8% <= BER < 1.6%
  CSQ_BER_4       = 4, // 1.6% <= BER < 3.2%
  CSQ_BER_5       = 5, // 3.2% <= BER < 6.4%
  CSQ_BER_6       = 6, // 6.4% <= BER < 12.8%
  CSQ_BER_7       = 7, // 12.8% <= BER
  CSQ_BER_UNKNOWN = 99 // Not known or not detectable
} csq_ber_value_t;
#define CSQ_BER_MAP_SIZE 9 // 0-7 plus UNKNOWN(99)
extern const enum_str_map_t CSQ_BER_MAP[CSQ_BER_MAP_SIZE];

// Helper fxns
int16_t csq_rssi_to_dbm(uint8_t rssi);
uint8_t csq_dbm_to_rssi(int16_t dbm);

extern const at_cmd_t AT_CMD_CSQ;
