// Return list of PDP addresses for a specified context id  (cid).
// If no cid is provided, addresses for all contexts are provided
#pragma once

#include "at_cmd_structure.h"

#define CGPADDR_ADDRESS_MAX_CHARS 18
#define CGPADDR_CID_RANGE_MIN_VALUE 0
#define CGPADDR_CID_RANGE_MAX_VALUE 15

typedef struct
{
  int  cid;
  char address[CGPADDR_ADDRESS_MAX_CHARS];
} cgpaddr_write_response_t;

typedef struct
{
  int cid;
} cgpaddr_write_params_t;

extern const at_cmd_t AT_CMD_CGPADDR;
