// DEFINE PDP CONTEXT AT CMD
#pragma once

/**
This command specifies PDP context parameters for a specific context <cid>. A special form of the
Write Command (AT+CGDCONT=<cid>) causes the values for context <cid> to become undefined. It is not
allowed to change the definition of an already activated context.
This Read Command returns the current settings for each defined PDP context.
*/

#include "at_cmd_structure.h"
#include "enum_utils.h"

#include <stdbool.h>
#include <stdint.h>

// PDP type definitions
typedef enum
{
  CGDCONT_PDP_TYPE_IP     = 0, // IPv4
  CGDCONT_PDP_TYPE_PPP    = 1, // PPP
  CGDCONT_PDP_TYPE_IPV6   = 2, // IPv6
  CGDCONT_PDP_TYPE_IPV4V6 = 3, // IPv4v6
  CGDCONT_PDP_TYPE_NON_IP = 4  // Non-IP
} cgdcont_pdp_type_t;

#define CGDCONT_PDP_TYPE_MAP_SIZE 5
extern const enum_str_map_t CGDCONT_PDP_TYPE_MAP[CGDCONT_PDP_TYPE_MAP_SIZE];

// Data compression modes
typedef enum
{
  CGDCONT_DATA_COMP_OFF    = 0, // OFF (Default)
  CGDCONT_DATA_COMP_ON     = 1, // ON (Manufacturer preferred)
  CGDCONT_DATA_COMP_V42BIS = 2  // V.42bis
} cgdcont_data_comp_t;

#define CGDCONT_DATA_COMP_MAP_SIZE 3
extern const enum_str_map_t CGDCONT_DATA_COMP_MAP[CGDCONT_DATA_COMP_MAP_SIZE];

// Header compression modes
typedef enum
{
  CGDCONT_HEAD_COMP_OFF     = 0, // OFF
  CGDCONT_HEAD_COMP_ON      = 1, // ON
  CGDCONT_HEAD_COMP_RFC1144 = 2, // RFC 1144
  CGDCONT_HEAD_COMP_RFC2507 = 3, // RFC 2507
  CGDCONT_HEAD_COMP_RFC3095 = 4  // RFC 3095
} cgdcont_head_comp_t;

#define CGDCONT_HEAD_COMP_MAP_SIZE 5
extern const enum_str_map_t CGDCONT_HEAD_COMP_MAP[CGDCONT_HEAD_COMP_MAP_SIZE];

// IPv4 address allocation methods
typedef enum
{
  CGDCONT_IPV4_ADDR_ALLOC_NAS = 0, // IPv4 address allocation through NAS signaling
} cgdcont_ipv4_addr_alloc_t;

#define CGDCONT_IPV4_ADDR_ALLOC_MAP_SIZE 1
extern const enum_str_map_t CGDCONT_IPV4_ADDR_ALLOC_MAP[CGDCONT_IPV4_ADDR_ALLOC_MAP_SIZE];

// Present flags structure for command responses
typedef struct
{
  bool has_cid : 1;
  bool has_pdp_type : 1;
  bool has_apn : 1;
  bool has_pdp_addr : 1;
  bool has_data_comp : 1;
  bool has_head_comp : 1;
  bool has_ipv4_addr_alloc : 1;
} cgdcont_present_flags_t;

// PDP context definition structure
typedef struct
{
  uint8_t                   cid;             // PDP context identifier (1-15)
  cgdcont_pdp_type_t        pdp_type;        // PDP type
  char                      apn[128];        // Access Point Name
  char                      pdp_addr[128];   // PDP address
  cgdcont_data_comp_t       data_comp;       // Data compression
  cgdcont_head_comp_t       head_comp;       // Header compression
  cgdcont_ipv4_addr_alloc_t ipv4_addr_alloc; // IPv4 address allocation
  cgdcont_present_flags_t   present;         // Flags for which fields are present
} cgdcont_pdp_context_t;

// Response structure for test command
// typedef struct
// {
//   uint8_t min_cid; // Minimum CID value
//   uint8_t max_cid; // Maximum CID value
//   bool    supports_pdp_type_ip;
//   bool    supports_pdp_type_ppp;
//   bool    supports_pdp_type_ipv6;
//   bool    supports_pdp_type_ipv4v6;
//   bool    supports_pdp_type_non_ip;
//   // Data compression support flags
//   bool supports_data_comp[CGDCONT_DATA_COMP_MAP_SIZE];
//   // Header compression support flags
//   bool supports_head_comp[CGDCONT_HEAD_COMP_MAP_SIZE];
//   // IPv4 address allocation method support flags
//   bool supports_ipv4_addr_alloc[CGDCONT_IPV4_ADDR_ALLOC_MAP_SIZE];
// } cgdcont_test_response_t;

// Response structure for read command
typedef struct
{
  cgdcont_pdp_context_t contexts[15]; // Array of defined PDP contexts (max 15)
  uint8_t               num_contexts; // Number of defined contexts
} cgdcont_read_response_t;

// Parameters structure for write command
typedef struct
{
  uint8_t                   cid;             // PDP context identifier (1-15)
  cgdcont_pdp_type_t        pdp_type;        // PDP type
  char                      apn[128];        // Access Point Name
  char                      pdp_addr[128];   // PDP address
  cgdcont_data_comp_t       data_comp;       // Data compression
  cgdcont_head_comp_t       head_comp;       // Header compression
  cgdcont_ipv4_addr_alloc_t ipv4_addr_alloc; // IPv4 address allocation
  cgdcont_present_flags_t   present;         // Flags for which fields are present
} cgdcont_write_params_t;

// Command declaration
extern const at_cmd_t AT_CMD_CGDCONT;
