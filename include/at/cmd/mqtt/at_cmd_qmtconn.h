// Connect a client to MQTT server
#pragma once
#include "at_cmd_structure.h"
#include "enum_utils.h"

#include <stdint.h>

// Result from connect operation
typedef enum
{
  QMTCONN_RESULT_SUCCESS        = 0, // Packet sent successfully and ACK received from server
  QMTCONN_RESULT_RETRANSMISSION = 1, // Packet retransmission
  QMTCONN_RESULT_FAILED_TO_SEND = 2  // Failed to send a packet
} qmtconn_result_t;

#define QMTCONN_RESULT_MAP_SIZE 3
extern const enum_str_map_t QMTCONN_RESULT_MAP[QMTCONN_RESULT_MAP_SIZE];

// MQTT connection state
typedef enum
{
  QMTCONN_STATE_INITIALIZING  = 1, // MQTT is initializing
  QMTCONN_STATE_CONNECTING    = 2, // MQTT is connecting
  QMTCONN_STATE_CONNECTED     = 3, // MQTT is connected
  QMTCONN_STATE_DISCONNECTING = 4  // MQTT is disconnecting
} qmtconn_state_t;

#define QMTCONN_STATE_MAP_SIZE 4
extern const enum_str_map_t QMTCONN_STATE_MAP[QMTCONN_STATE_MAP_SIZE];

// Connection status return codes
typedef enum
{
  QMTCONN_RET_CODE_ACCEPTED              = 0, // Connection Accepted
  QMTCONN_RET_CODE_UNACCEPTABLE_PROTOCOL = 1, // Connection Refused: Unacceptable Protocol Version
  QMTCONN_RET_CODE_IDENTIFIER_REJECTED   = 2, // Connection Refused: Identifier Rejected
  QMTCONN_RET_CODE_SERVER_UNAVAILABLE    = 3, // Connection Refused: Server Unavailable
  QMTCONN_RET_CODE_BAD_CREDENTIALS       = 4, // Connection Refused: Bad Username or Password
  QMTCONN_RET_CODE_NOT_AUTHORIZED        = 5  // Connection Refused: Not Authorized
} qmtconn_ret_code_t;

#define QMTCONN_RET_CODE_MAP_SIZE 6
extern const enum_str_map_t QMTCONN_RET_CODE_MAP[QMTCONN_RET_CODE_MAP_SIZE];

#define QMTCONN_CLIENT_IDX_MIN 0
#define QMTCONN_CLIENT_IDX_MAX 5
#define QMTCONN_CLIENT_ID_MAX_SIZE 256
#define QMTCONN_USERNAME_MAX_SIZE 512
#define QMTCONN_PASSWORD_MAX_SIZE 512

// Present flags structure for responses
typedef struct
{
  bool has_client_idx : 1;
  bool has_state : 1;
  bool has_result : 1;
  bool has_ret_code : 1;
} qmtconn_present_flags_t;

// QMTCONN test response - for range of supported parameters
typedef struct
{
  uint8_t client_idx_min; // Minimum client index
  uint8_t client_idx_max; // Maximum client index
} qmtconn_test_response_t;

// QMTCONN read response - for querying connection status
typedef struct
{
  uint8_t                 client_idx; // MQTT client identifier (0-5)
  qmtconn_state_t         state;      // MQTT connection state
  qmtconn_present_flags_t present;    // Flags for which fields are present
} qmtconn_read_response_t;

// QMTCONN write parameters - for connecting to MQTT server
typedef struct
{
  uint8_t client_idx;                                // MQTT client identifier (0-5)
  char    client_id[QMTCONN_CLIENT_ID_MAX_SIZE + 1]; // Client identifier string
  char    username[QMTCONN_USERNAME_MAX_SIZE + 1];   // Client username (optional)
  char    password[QMTCONN_PASSWORD_MAX_SIZE + 1];   // Client password (optional)
  struct
  {
    bool has_username : 1; // Whether username is provided
    bool has_password : 1; // Whether password is provided
  } present;
} qmtconn_write_params_t;

// QMTCONN write response - async response after connection attempt
typedef struct
{
  uint8_t                 client_idx; // MQTT client identifier (0-5)
  qmtconn_result_t        result;     // Command execution result
  qmtconn_ret_code_t      ret_code;   // Connection status return code
  qmtconn_present_flags_t present;    // Flags for which fields are present
} qmtconn_write_response_t;

// Command declaration
extern const at_cmd_t AT_CMD_QMTCONN;
