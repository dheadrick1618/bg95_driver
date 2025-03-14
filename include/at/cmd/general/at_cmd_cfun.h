// Set the functionality level of the device. Also used for soft resets of the device.
#pragma once

#include "at_cmd_structure.h"
#include "enum_utils.h"

typedef enum
{
  CFUN_FUN_TYPE_MINIMUM           = 0U,
  CFUN_FUN_TYPE_FULL              = 1U,
  CFUN_FUN_TYPE_DISABLE_TX_AND_RX = 4U,
} cfun_fun_type_t;

#define CFUN_FUN_TYPE_MAP_SIZE 3
extern const enum_str_map_t CFUN_FUN_TYPE_MAP[CFUN_FUN_TYPE_MAP_SIZE];

// TODO: add reset param
//  typedef enum
//  {
//    CFUN_RST_TYPE_DO_NOT_RESET_UE_BEFORE_SETTING_FUN = 0U,
//  	CFUN_RST_TYPE_
//  } cfun_rst_type;
//

typedef struct
{
  cfun_fun_type_t fun_type;
} cfun_read_response_t;

typedef struct
{
  cfun_fun_type_t fun_type;
  // TODO: add reset param
} cfun_write_params_t;

extern const at_cmd_t AT_CMD_CFUN;
