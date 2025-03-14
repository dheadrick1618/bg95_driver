// Activate or Deactivate the specified PDP context.
#pragma once

#include "at_cmd_structure.h"
#include "enum_utils.h"

#define CGACT_MAX_NUM_CONTEXTS 15

typedef enum
{
  CGACT_STATE_DEACTIVATED = 0U,
  CGACT_STATE_ACTIVATED   = 1U
} cgact_state_t;
#define CGACT_STATE_MAP_SIZE 2
extern const enum_str_map_t CGACT_STATE_MAP[CGACT_STATE_MAP_SIZE];

typedef struct
{
  int           cid;
  cgact_state_t state;
} cgact_context_t;

typedef struct
{
  cgact_context_t contexts[CGACT_MAX_NUM_CONTEXTS];
  int             num_contexts;
} cgact_read_response_t;

typedef struct
{
  cgact_state_t state;
  int           cid;
} cgact_write_params_t;

const extern at_cmd_t AT_CMD_CGACT;
