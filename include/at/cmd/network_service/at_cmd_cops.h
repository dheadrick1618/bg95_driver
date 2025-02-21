#pragma once

#include "at_cmd_structure.h"
#include "enum_utils.h"

#include <stdbool.h>

// COPS command types
typedef enum
{
  COPS_STAT_UNKNOWN            = 0U,
  COPS_STAT_OPERATOR_AVAILABLE = 1U,
  COPS_STAT_CURRENT_OPERATOR   = 2U,
  COPS_STAT_OPERATOR_FORBIDDEN = 3U,
} cops_stat_t;
#define COPS_STAT_MAP_SIZE 4
extern const enum_str_map_t COPS_STAT_MAP[COPS_STAT_MAP_SIZE];

typedef enum
{
  COPS_MODE_AUTO        = 0U,
  COPS_MODE_MANUAL      = 1U,
  COPS_MODE_DEREGISTER  = 2U,
  COPS_MODE_SET_FORMAT  = 3U,
  COPS_MODE_MANUAL_AUTO = 4U,
} cops_mode_t;
#define COPS_MODE_MAP_SIZE 5
extern const enum_str_map_t COPS_MODE_MAP[COPS_MODE_MAP_SIZE];

typedef enum
{
  COPS_FORMAT_LONG_ALPHA  = 0U,
  COPS_FORMAT_SHORT_ALPHA = 1U,
  COPS_FORMAT_NUMERIC     = 2U,
} cops_format_t;
#define COPS_FORMAT_MAP_SIZE 3
extern const enum_str_map_t COPS_FORMAT_MAP[COPS_FORMAT_MAP_SIZE];

typedef enum
{
  COPS_ACT_GSM    = 0U,
  COPS_ACT_EMTC   = 8U,
  COPS_ACT_NB_IOT = 9U,
} cops_act_t;
#define COPS_ACT_MAP_SIZE 3
extern const enum_str_map_t COPS_ACT_MAP[COPS_ACT_MAP_SIZE];

// TODO: Implement the test type of this AT cmd
//  Response structures
//  typedef struct
//  {
//    cops_stat_t stat;
//    char        operator_name[64];
//    cops_act_t  act;
//    struct
//    {
//      bool has_stat : 1;
//      bool has_operator : 1;
//      bool has_act : 1;
//    } present;
//  } cops_operator_info_t;
//
//  typedef struct
//  {
//    cops_operator_info_t operators[10];
//    uint8_t              num_operators;
//  } cops_test_response_t;

// This is the struct used by the bg95 driver fxn , so it doesn't use a response or parameter struct
// for parsing or formatting only
typedef struct
{
  cops_mode_t   mode;
  cops_format_t format;
  char          operator_name[64];
  cops_act_t    act;
} cops_operator_data_t;

// Present flags structure for command responses
typedef struct
{
  bool has_mode : 1;
  bool has_format : 1;
  bool has_operator : 1;
  bool has_act : 1;
} cops_present_flags_t;

// For read command response
typedef struct
{
  cops_mode_t          mode;
  cops_format_t        format;
  char                 operator_name[64];
  cops_act_t           act;
  cops_present_flags_t present;
} cops_read_response_t;

// // For write command parameters
// typedef struct
// {
//   cops_mode_t          mode;
//   cops_format_t        format;
//   char                 operator_name[64];
//   cops_act_t           act;
//   cops_present_flags_t present;
// } cops_write_params_t;

extern const at_cmd_t AT_CMD_COPS;
