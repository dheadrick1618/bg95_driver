#pragma once

#include "at_cmd_structure.h"

#include <stdbool.h>
#include <stdint.h>

// Signal quality response structure
typedef struct {
    int8_t rssi;           // Raw RSSI value (0-31, 99)
    int8_t ber;            // Bit Error Rate (0-7, 99)
    int16_t rssi_dbm;      // Converted RSSI value in dBm
    struct {
        bool has_rssi: 1;
        bool has_ber: 1;
    } present;
} csq_response_t;

// Test response structure
typedef struct {
    uint8_t rssi_min;      // Minimum supported RSSI value
    uint8_t rssi_max;      // Maximum supported RSSI value
    uint8_t ber_min;       // Minimum supported BER value
    uint8_t ber_max;       // Maximum supported BER value
    bool rssi_unknown;     // Whether 99 (unknown) is supported for RSSI
    bool ber_unknown;      // Whether 99 (unknown) is supported for BER
} csq_test_response_t;

// Command declaration
extern const at_cmd_t AT_CMD_CSQ;

// Helper function to convert RSSI value to dBm
int16_t csq_rssi_to_dbm(uint8_t rssi);
