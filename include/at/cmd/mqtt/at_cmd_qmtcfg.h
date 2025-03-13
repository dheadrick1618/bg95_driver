// Configure optional parameters of MQTT
#pragma once

#include "at_cmd_structure.h"
#include "enum_utils.h"

#include <stdbool.h>
#include <stdint.h>

// MQTT Protocol Version
typedef enum
{
  QMTCFG_VERSION_MQTT_3_1   = 3U, // MQTT v3.1
  QMTCFG_VERSION_MQTT_3_1_1 = 4U  // MQTT v3.1.1
} qmtcfg_version_t;
#define QMTCFG_VERSION_MAP_SIZE 2
extern const enum_str_map_t QMTCFG_VERSION_MAP[QMTCFG_VERSION_MAP_SIZE];

// SSL Mode
typedef enum
{
  QMTCFG_SSL_DISABLE = 0U, // Use normal TCP connection for MQTT
  QMTCFG_SSL_ENABLE  = 1U  // Use SSL TCP secure connection for MQTT
} qmtcfg_ssl_mode_t;
#define QMTCFG_SSL_MODE_MAP_SIZE 2
extern const enum_str_map_t QMTCFG_SSL_MODE_MAP[QMTCFG_SSL_MODE_MAP_SIZE];

// Clean session mode
typedef enum
{
  QMTCFG_CLEAN_SESSION_DISABLE = 0U, // Server must store subscriptions after client disconnect
  QMTCFG_CLEAN_SESSION_ENABLE  = 1U  // Server must discard previous info after disconnect
} qmtcfg_clean_session_t;
#define QMTCFG_CLEAN_SESSION_MAP_SIZE 2
extern const enum_str_map_t QMTCFG_CLEAN_SESSION_MAP[QMTCFG_CLEAN_SESSION_MAP_SIZE];

// Will flag
typedef enum
{
  QMTCFG_WILL_FLAG_IGNORE  = 0U, // Ignore will flag configuration
  QMTCFG_WILL_FLAG_REQUIRE = 1U  // Require will flag configuration
} qmtcfg_will_flag_t;
#define QMTCFG_WILL_FLAG_MAP_SIZE 2
extern const enum_str_map_t QMTCFG_WILL_FLAG_MAP[QMTCFG_WILL_FLAG_MAP_SIZE];

// Will QoS levels
typedef enum
{
  QMTCFG_WILL_QOS_0 = 0U, // At most once
  QMTCFG_WILL_QOS_1 = 1U, // At least once
  QMTCFG_WILL_QOS_2 = 2U  // Exactly once
} qmtcfg_will_qos_t;
#define QMTCFG_WILL_QOS_MAP_SIZE 3
extern const enum_str_map_t QMTCFG_WILL_QOS_MAP[QMTCFG_WILL_QOS_MAP_SIZE];

// Will retain
typedef enum
{
  QMTCFG_WILL_RETAIN_DISABLE = 0U, // Don't retain will message
  QMTCFG_WILL_RETAIN_ENABLE  = 1U  // Retain will message after delivery
} qmtcfg_will_retain_t;
#define QMTCFG_WILL_RETAIN_MAP_SIZE 2
extern const enum_str_map_t QMTCFG_WILL_RETAIN_MAP[QMTCFG_WILL_RETAIN_MAP_SIZE];

// Message receive mode
typedef enum
{
  QMTCFG_MSG_RECV_MODE_CONTAIN_IN_URC     = 0U, // Message received will be contained in URC
  QMTCFG_MSG_RECV_MODE_NOT_CONTAIN_IN_URC = 1U  // Message received will not be contained in URC
} qmtcfg_msg_recv_mode_t;
#define QMTCFG_MSG_RECV_MODE_MAP_SIZE 2
extern const enum_str_map_t QMTCFG_MSG_RECV_MODE_MAP[QMTCFG_MSG_RECV_MODE_MAP_SIZE];

// Message length enable
typedef enum
{
  QMTCFG_MSG_LEN_DISABLE = 0U, // Length will not be contained in URC
  QMTCFG_MSG_LEN_ENABLE  = 1U  // Length will be contained in URC
} qmtcfg_msg_len_enable_t;
#define QMTCFG_MSG_LEN_ENABLE_MAP_SIZE 2
extern const enum_str_map_t QMTCFG_MSG_LEN_ENABLE_MAP[QMTCFG_MSG_LEN_ENABLE_MAP_SIZE];

// Timeout notice
typedef enum
{
  QMTCFG_TIMEOUT_NOTICE_DISABLE = 0U, // Do not report
  QMTCFG_TIMEOUT_NOTICE_ENABLE  = 1U  // Report
} qmtcfg_timeout_notice_t;
#define QMTCFG_TIMEOUT_NOTICE_MAP_SIZE 2
extern const enum_str_map_t QMTCFG_TIMEOUT_NOTICE_MAP[QMTCFG_TIMEOUT_NOTICE_MAP_SIZE];

// Command sub-types
typedef enum
{
  QMTCFG_TYPE_VERSION   = 0U,
  QMTCFG_TYPE_PDPCID    = 1U,
  QMTCFG_TYPE_SSL       = 2U,
  QMTCFG_TYPE_KEEPALIVE = 3U,
  QMTCFG_TYPE_SESSION   = 4U,
  QMTCFG_TYPE_TIMEOUT   = 5U,
  QMTCFG_TYPE_WILL      = 6U,
  QMTCFG_TYPE_RECV_MODE = 7U,
  QMTCFG_TYPE_ALIAUTH   = 8U,
  QMTCFG_TYPE_MAX       = 9U
} qmtcfg_type_t;
#define QMTCFG_TYPE_MAP_SIZE 9
extern const enum_str_map_t QMTCFG_TYPE_MAP[QMTCFG_TYPE_MAP_SIZE];

// Test command response structure
typedef struct
{
  // Lists of supported values shown by the test command
  uint8_t  client_idx_min;
  uint8_t  client_idx_max;
  uint8_t  supported_versions[2];
  uint8_t  num_supported_versions;
  uint8_t  pdp_cid_min;
  uint8_t  pdp_cid_max;
  bool     supports_ssl_modes[QMTCFG_SSL_MODE_MAP_SIZE];
  uint8_t  ctx_index_min;
  uint8_t  ctx_index_max;
  uint16_t keep_alive_min;
  uint16_t keep_alive_max;
  bool     supports_clean_session_modes[QMTCFG_CLEAN_SESSION_MAP_SIZE];
  uint8_t  pkt_timeout_min;
  uint8_t  pkt_timeout_max;
  uint8_t  retry_times_min;
  uint8_t  retry_times_max;
  bool     supports_timeout_notice_modes[QMTCFG_TIMEOUT_NOTICE_MAP_SIZE];
  bool     supports_will_flags[QMTCFG_WILL_FLAG_MAP_SIZE];
  bool     supports_will_qos[QMTCFG_WILL_QOS_MAP_SIZE];
  bool     supports_will_retain[QMTCFG_WILL_RETAIN_MAP_SIZE];
  bool     supports_msg_recv_modes[QMTCFG_MSG_RECV_MODE_MAP_SIZE];
  bool     supports_msg_len_modes[QMTCFG_MSG_LEN_ENABLE_MAP_SIZE];
} qmtcfg_test_response_t;

// ------- Version configuration -------
typedef struct
{
  uint8_t          client_idx; // MQTT client identifier (0-5)
  qmtcfg_version_t version;    // MQTT protocol version
  struct
  {
    bool has_version : 1;
  } present;
} qmtcfg_write_version_params_t;

// ------- PDP CID configuration -------
typedef struct
{
  uint8_t client_idx; // MQTT client identifier (0-5)
  uint8_t pdp_cid;    // The PDP to be used by MQTT client (1-16)
  struct
  {
    bool has_pdp_cid : 1;
  } present;
} qmtcfg_write_pdpcid_params_t;

// ------- SSL configuration -------
typedef struct
{
  uint8_t           client_idx; // MQTT client identifier (0-5)
  qmtcfg_ssl_mode_t ssl_enable; // MQTT SSL mode
  uint8_t           ctx_index;  // SSL context index (0-5)
  struct
  {
    bool has_ssl_enable : 1;
    bool has_ctx_index : 1;
  } present;
} qmtcfg_write_ssl_params_t;

// ------- Keep-alive configuration -------
typedef struct
{
  uint8_t  client_idx;      // MQTT client identifier (0-5)
  uint16_t keep_alive_time; // Keep-alive time in seconds (0-3600)
  struct
  {
    bool has_keep_alive_time : 1;
  } present;
} qmtcfg_write_keepalive_params_t;

// ------- Session configuration -------
typedef struct
{
  uint8_t                client_idx;    // MQTT client identifier (0-5)
  qmtcfg_clean_session_t clean_session; // Session type
  struct
  {
    bool has_clean_session : 1;
  } present;
} qmtcfg_write_session_params_t;

// ------- Timeout configuration -------
typedef struct
{
  uint8_t                 client_idx;     // MQTT client identifier (0-5)
  uint8_t                 pkt_timeout;    // Packet delivery timeout in seconds (1-60)
  uint8_t                 retry_times;    // Retry times (0-10)
  qmtcfg_timeout_notice_t timeout_notice; // Whether to report timeout
  struct
  {
    bool has_pkt_timeout : 1;
    bool has_retry_times : 1;
    bool has_timeout_notice : 1;
  } present;
} qmtcfg_write_timeout_params_t;

// ------- Will configuration -------
typedef struct
{
  uint8_t              client_idx;        // MQTT client identifier (0-5)
  qmtcfg_will_flag_t   will_flag;         // Will flag
  qmtcfg_will_qos_t    will_qos;          // Will QoS level
  qmtcfg_will_retain_t will_retain;       // Will retain flag
  char                 will_topic[256];   // Will topic (max 255 bytes)
  char                 will_message[256]; // Will message (max 255 bytes)
  struct
  {
    bool has_will_flag : 1;
    bool has_will_qos : 1;
    bool has_will_retain : 1;
    bool has_will_topic : 1;
    bool has_will_message : 1;
  } present;
} qmtcfg_write_will_params_t;

// ------- Receive mode configuration -------
typedef struct
{
  uint8_t                 client_idx;     // MQTT client identifier (0-5)
  qmtcfg_msg_recv_mode_t  msg_recv_mode;  // Message receive mode
  qmtcfg_msg_len_enable_t msg_len_enable; // Message length enable
  struct
  {
    bool has_msg_recv_mode : 1;
    bool has_msg_len_enable : 1;
  } present;
} qmtcfg_write_recv_mode_params_t;

// ------- Response structures for write commands -------

// Version response
typedef struct
{
  qmtcfg_version_t version; // MQTT protocol version
  struct
  {
    bool has_version : 1;
  } present;
} qmtcfg_write_version_response_t;

// PDP CID response
typedef struct
{
  uint8_t pdp_cid; // The PDP to be used by MQTT client (1-16)
  struct
  {
    bool has_pdp_cid : 1;
  } present;
} qmtcfg_write_pdpcid_response_t;

// SSL configuration response
typedef struct
{
  qmtcfg_ssl_mode_t ssl_enable; // MQTT SSL mode
  uint8_t           ctx_index;  // SSL context index (0-5)
  struct
  {
    bool has_ssl_enable : 1;
    bool has_ctx_index : 1;
  } present;
} qmtcfg_write_ssl_response_t;

// Keep-alive configuration response
typedef struct
{
  uint16_t keep_alive_time; // Keep-alive time in seconds (0-3600)
  struct
  {
    bool has_keep_alive_time : 1;
  } present;
} qmtcfg_write_keepalive_response_t;

// Session configuration response
typedef struct
{
  qmtcfg_clean_session_t clean_session; // Session type
  struct
  {
    bool has_clean_session : 1;
  } present;
} qmtcfg_write_session_response_t;

// Timeout configuration response
typedef struct
{
  uint8_t                 pkt_timeout;    // Packet delivery timeout in seconds (1-60)
  uint8_t                 retry_times;    // Retry times (0-10)
  qmtcfg_timeout_notice_t timeout_notice; // Whether to report timeout
  struct
  {
    bool has_pkt_timeout : 1;
    bool has_retry_times : 1;
    bool has_timeout_notice : 1;
  } present;
} qmtcfg_write_timeout_response_t;

// Will configuration response
typedef struct
{
  qmtcfg_will_flag_t   will_flag;         // Will flag
  qmtcfg_will_qos_t    will_qos;          // Will QoS level
  qmtcfg_will_retain_t will_retain;       // Will retain flag
  char                 will_topic[256];   // Will topic (max 255 bytes)
  char                 will_message[256]; // Will message (max 255 bytes)
  struct
  {
    bool has_will_flag : 1;
    bool has_will_qos : 1;
    bool has_will_retain : 1;
    bool has_will_topic : 1;
    bool has_will_message : 1;
  } present;
} qmtcfg_write_will_response_t;

// Receive mode configuration response
typedef struct
{
  qmtcfg_msg_recv_mode_t  msg_recv_mode;  // Message receive mode
  qmtcfg_msg_len_enable_t msg_len_enable; // Message length enable
  struct
  {
    bool has_msg_recv_mode : 1;
    bool has_msg_len_enable : 1;
  } present;
} qmtcfg_write_recv_mode_response_t;

// Generic write parameters structure for the AT command handler
typedef struct
{
  qmtcfg_type_t type;
  union
  {
    qmtcfg_write_version_params_t   version;
    qmtcfg_write_pdpcid_params_t    pdpcid;
    qmtcfg_write_ssl_params_t       ssl;
    qmtcfg_write_keepalive_params_t keepalive;
    qmtcfg_write_session_params_t   session;
    qmtcfg_write_timeout_params_t   timeout;
    qmtcfg_write_will_params_t      will;
    qmtcfg_write_recv_mode_params_t recv_mode;
  } params;
} qmtcfg_write_params_t;

// Generic write response structure
typedef struct
{
  qmtcfg_type_t type;
  union
  {
    qmtcfg_write_version_response_t   version;
    qmtcfg_write_pdpcid_response_t    pdpcid;
    qmtcfg_write_ssl_response_t       ssl;
    qmtcfg_write_keepalive_response_t keepalive;
    qmtcfg_write_session_response_t   session;
    qmtcfg_write_timeout_response_t   timeout;
    qmtcfg_write_will_response_t      will;
    qmtcfg_write_recv_mode_response_t recv_mode;
  } response;
} qmtcfg_write_response_t;

// Command declaration
extern const at_cmd_t AT_CMD_QMTCFG;
