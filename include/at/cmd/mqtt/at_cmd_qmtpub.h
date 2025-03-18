// Publish Messages to MQTT Server in Data Mode
#pragma once
#include "at_cmd_structure.h"
#include "enum_utils.h"

#include <stdint.h>

// Result from MQTT publish operation
typedef enum
{
  QMTPUB_RESULT_SUCCESS        = 0, // Packet sent successfully and ACK received
  QMTPUB_RESULT_RETRANSMISSION = 1, // Packet retransmission
  QMTPUB_RESULT_FAILED_TO_SEND = 2  // Failed to send a packet
} qmtpub_result_t;

#define QMTPUB_RESULT_MAP_SIZE 3
extern const enum_str_map_t QMTPUB_RESULT_MAP[QMTPUB_RESULT_MAP_SIZE];

// QoS levels
typedef enum
{
  QMTPUB_QOS_AT_MOST_ONCE  = 0, // At most once
  QMTPUB_QOS_AT_LEAST_ONCE = 1, // At least once
  QMTPUB_QOS_EXACTLY_ONCE  = 2  // Exactly once
} qmtpub_qos_t;

#define QMTPUB_QOS_MAP_SIZE 3
extern const enum_str_map_t QMTPUB_QOS_MAP[QMTPUB_QOS_MAP_SIZE];

// Retain flag values
typedef enum
{
  QMTPUB_RETAIN_DISABLED = 0, // The server will not retain the message
  QMTPUB_RETAIN_ENABLED  = 1  // The server will retain the message
} qmtpub_retain_t;

#define QMTPUB_RETAIN_MAP_SIZE 2
extern const enum_str_map_t QMTPUB_RETAIN_MAP[QMTPUB_RETAIN_MAP_SIZE];

#define QMTPUB_CLIENT_IDX_MIN 0
#define QMTPUB_CLIENT_IDX_MAX 5
#define QMTPUB_MSGID_MIN 0
#define QMTPUB_MSGID_MAX 65535
#define QMTPUB_TOPIC_MAX_SIZE 128
#define QMTPUB_MSG_MAX_LEN 4096

// Present flags structure for responses
typedef struct
{
  bool has_client_idx : 1;
  bool has_msgid : 1;
  bool has_result : 1;
  bool has_value : 1;
} qmtpub_present_flags_t;

// QMTPUB write parameters - for fixed-length publish
typedef struct
{
  uint8_t         client_idx;                   // MQTT client identifier (0-5)
  uint16_t        msgid;                        // Message identifier (0-65535)
  qmtpub_qos_t    qos;                          // QoS level
  qmtpub_retain_t retain;                       // Retain flag
  char            topic[QMTPUB_TOPIC_MAX_SIZE]; // Topic to publish to
  uint16_t        msglen;                       // Message length (required for fixed-length)
} qmtpub_write_params_t;

// QMTPUB write response - response after publication
typedef struct
{
  uint8_t                client_idx; // MQTT client identifier (0-5)
  uint16_t               msgid;      // Message identifier (0-65535)
  qmtpub_result_t        result;     // Command execution result
  uint8_t                value;      // Number of retransmissions if result=1
  qmtpub_present_flags_t present;    // Flags for which fields are present
} qmtpub_write_response_t;

// Command declaration
extern const at_cmd_t AT_CMD_QMTPUB;
