#pragma once
#include "at_cmd_structure.h"
#include "enum_utils.h"

#include <stdint.h>

// Result from unsubscribe operation
typedef enum
{
  QMTUNS_RESULT_SUCCESS        = 0, // Packet sent successfully and ACK received
  QMTUNS_RESULT_RETRANSMISSION = 1, // Packet retransmission
  QMTUNS_RESULT_FAILED_TO_SEND = 2  // Failed to send a packet
} qmtuns_result_t;

#define QMTUNS_RESULT_MAP_SIZE 3
extern const enum_str_map_t QMTUNS_RESULT_MAP[QMTUNS_RESULT_MAP_SIZE];

#define QMTUNS_CLIENT_IDX_MIN 0
#define QMTUNS_CLIENT_IDX_MAX 5
#define QMTUNS_MSGID_MIN 1
#define QMTUNS_MSGID_MAX 65535
#define QMTUNS_TOPIC_MAX_SIZE 128
#define QMTUNS_MAX_TOPICS 5 // Maximum topics in one unsubscribe command

// Present flags structure for responses
typedef struct
{
  bool has_client_idx : 1;
  bool has_msgid : 1;
  bool has_result : 1;
} qmtuns_present_flags_t;

// QMTUNS test response - for range of supported client indexes
typedef struct
{
  uint8_t  client_idx_min; // Minimum client index
  uint8_t  client_idx_max; // Maximum client index
  uint16_t msgid_min;      // Minimum message ID
  uint16_t msgid_max;      // Maximum message ID
} qmtuns_test_response_t;

// QMTUNS write parameters - for unsubscribing from topics
typedef struct
{
  uint8_t  client_idx;                                       // MQTT client identifier (0-5)
  uint16_t msgid;                                            // Message identifier (1-65535)
  uint8_t  topic_count;                                      // Number of topics to unsubscribe
  char     topics[QMTUNS_MAX_TOPICS][QMTUNS_TOPIC_MAX_SIZE]; // Topics to unsubscribe from
} qmtuns_write_params_t;

// QMTUNS write response - response after unsubscription
typedef struct
{
  uint8_t                client_idx; // MQTT client identifier (0-5)
  uint16_t               msgid;      // Message identifier (1-65535)
  qmtuns_result_t        result;     // Command execution result
  qmtuns_present_flags_t present;    // Flags for which fields are present
} qmtuns_write_response_t;

// Command declaration
extern const at_cmd_t AT_CMD_QMTUNS;
