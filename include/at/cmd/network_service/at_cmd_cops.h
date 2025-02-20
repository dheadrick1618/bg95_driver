#pragma once

#include "at_cmd_structure.h"
#include <stdbool.h>

// COPS command types
typedef enum {
    COPS_STAT_UNKNOWN = 0,
    COPS_STAT_CURRENT_OPERATOR = 1,
    COPS_STAT_OPERATOR_FORBIDDEN = 2,
    COPS_STAT_MAX
} cops_stat_t;

typedef enum {
    COPS_MODE_AUTO = 0,        
    COPS_MODE_MANUAL = 1,      
    COPS_MODE_DEREGISTER = 2,  
    COPS_MODE_SET_FORMAT = 3,  
    COPS_MODE_MANUAL_AUTO = 4, 
    COPS_MODE_MAX = 5
} cops_mode_t;

typedef enum {
    COPS_FORMAT_LONG = 0,    
    COPS_FORMAT_SHORT = 1,   
    COPS_FORMAT_NUMERIC = 2, 
    COPS_FORMAT_MAX = 3
} cops_format_t;

typedef enum {
    COPS_ACT_GSM = 0,
    COPS_ACT_EMTC = 1,
    COPS_ACT_NB_IOT = 2,
    COPS_ACT_MAX = 3
} cops_act_t;

// Response structures
typedef struct {
    cops_stat_t stat;
    char operator_name[64];
    cops_act_t act;
    struct {
        bool has_stat: 1;
        bool has_operator: 1;
        bool has_act: 1;
    } present;
} cops_operator_info_t;

typedef struct {
    cops_operator_info_t operators[10];
    uint8_t num_operators;
} cops_test_response_t;

typedef struct {
    cops_mode_t mode;
    cops_format_t format;
    char operator_name[64];
    cops_act_t act;
    struct {
        bool has_mode: 1;
        bool has_format: 1;
        bool has_operator: 1;
        bool has_act: 1;
    } present;
} cops_read_response_t;

typedef struct {
    cops_mode_t mode;
    cops_format_t format;
    char operator_name[64];
    cops_act_t act;
} cops_write_params_t;

// Command declaration
extern const at_cmd_t AT_CMD_COPS;
