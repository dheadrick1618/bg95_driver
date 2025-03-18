// include/at/cmd/mqtt/at_cmd_qmtsub.h
#pragma once
#include "at_cmd_structure.h"
#include "enum_utils.h"

#include <stdint.h>

// Result from subscribe operation
typedef enum
{
  QMTSUB_RESULT_SUCCESS        = 0, // Packet sent successfully and ACK received
  QMTSUB_RESULT_RETRANSMISSION = 1, // Packet retransmission
  QMTSUB_RESULT_FAILED_TO_SEND = 2  // Failed to send a packet
} qmtsub_result_t;

#define QMTSUB_RESULT_MAP_SIZE 3
extern const enum_str_map_t QMTSUB_RESULT_MAP[QMTSUB_RESULT_MAP_SIZE];

// QoS levels
typedef enum
{
  QMTSUB_QOS_AT_MOST_ONCE  = 0, // At most once
  QMTSUB_QOS_AT_LEAST_ONCE = 1, // At least once
  QMTSUB_QOS_EXACTLY_ONCE  = 2  // Exactly once
} qmtsub_qos_t;

#define QMTSUB_QOS_MAP_SIZE 3
extern const enum_str_map_t QMTSUB_QOS_MAP[QMTSUB_QOS_MAP_SIZE];

#define QMTSUB_CLIENT_IDX_MIN 0
#define QMTSUB_CLIENT_IDX_MAX 5
#define QMTSUB_MSGID_MIN 1
#define QMTSUB_MSGID_MAX 65535
#define QMTSUB_TOPIC_MAX_SIZE 128
#define QMTSUB_MAX_TOPICS 5 // Maximum topics in one subscribe command

// Present flags structure for responses
typedef struct
{
  bool has_client_idx : 1;
  bool has_msgid : 1;
  bool has_result : 1;
  bool has_value : 1;
} qmtsub_present_flags_t;

// Structure for a single topic filter and QoS pair
typedef struct
{
  char         topic[QMTSUB_TOPIC_MAX_SIZE];
  qmtsub_qos_t qos;
} qmtsub_topic_filter_t;

// QMTSUB test response - for range of supported client indexes
typedef struct
{
  uint8_t  client_idx_min; // Minimum client index
  uint8_t  client_idx_max; // Maximum client index
  uint16_t msgid_min;      // Minimum message ID
  uint16_t msgid_max;      // Maximum message ID
  uint8_t  qos_min;        // Minimum QoS level
  uint8_t  qos_max;        // Maximum QoS level
} qmtsub_test_response_t;

// QMTSUB write parameters - for subscribing to topics
typedef struct
{
  uint8_t               client_idx;                // MQTT client identifier (0-5)
  uint16_t              msgid;                     // Message identifier (1-65535)
  uint8_t               topic_count;               // Number of topics to subscribe
  qmtsub_topic_filter_t topics[QMTSUB_MAX_TOPICS]; // Topics to subscribe to
} qmtsub_write_params_t;

// QMTSUB write response - response after subscription
typedef struct
{
  uint8_t                client_idx; // MQTT client identifier (0-5)
  uint16_t               msgid;      // Message identifier (1-65535)
  qmtsub_result_t        result;     // Command execution result
  uint8_t                value;      // Granted QoS or retransmission count
  qmtsub_present_flags_t present;    // Flags for which fields are present
} qmtsub_write_response_t;

// Command declaration
extern const at_cmd_t AT_CMD_QMTSUB;
