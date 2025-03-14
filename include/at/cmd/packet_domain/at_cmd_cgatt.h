// Attach or Detach from Packet Domain / Packet Switching service (PS)
#pragma once

#include "at_cmd_structure.h"

typedef enum
{
  CGATT_STATE_DETACHED = 0U,
  CGATT_STATE_ATTACHED = 1U,
  CGATT_STATE_MAX      = 2U
} cgatt_state_t;

// NOTE: dont  care about test cmd or its  params

typedef struct
{
  cgatt_state_t state;
} cgatt_read_params_t;

typedef struct
{
  cgatt_state_t state;
} cgatt_write_params_t;

extern const at_cmd_t AT_CMD_CGATT;
