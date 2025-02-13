// Configure MQTT parameters on device
#pragma once
#include <stdint.h>

typedef enum
{
  QMTCFG_MQTT_PROTOCOL_VER_3_1   = 3U,
  QMTCFG_MQTT_PROTOCOL_VER_3_1_1 = 4U
} qmtcfg_mqtt_protocol_version_t;

// NOTE: CID - What PDP to be used by MQTT client

typedef enum
{
  QMTCFG_MQTT_WILL_FLAG_IGNORE  = 0U,
  QMTCFG_MQTT_WILL_FLAG_REQUIRE = 1U
} qmtcfg_mqtt_will_flag_setting_t;

typedef enum
{
  QMTCFG_MQTT_QOS_0 = 0U,
  QMTCFG_MQTT_QOS_1 = 1U,
  QMTCFG_MQTT_QOS_2 = 2U
} qmtcfg_mqtt_qos_t;

// TODO: will retain
// TODO: will message
// TODO: pkt timeout  (delivery timeout in seconds)
// TODO: retry times
// TODO: timeout notice

typedef enum
{
  QMTCFG_MQTT_CLEAN_SESSION_DISABLE = 0U,
  QMTCFG_MQTT_CLEAN_SESSION_ENABLE  = 1U
} qmtcfg_mqtt_clean_session_t;

// NOTE: keep alive time (in seconds)

typedef enum
{
  QMTCFG_MQTT_SSL_DISABLE = 0U, // Use Normal TCP connection
  QMTCFG_MQTT_SSL_ENABLE  = 1U  // Use SSL TCP secure connection
} qmtcfg_mqtt_ssl_toggle_t;

// NOTE: ctx_index (ssl context  index 0-5)
// TODO: msg recv mode (mqtt message receiving mode)
// TODO: msg len enable  (is len of mqtt msg contained in URC?)
// NOTE: product key (issued by ali cloud )

typedef struct
{
  qmtcfg_mqtt_protocol_version_t version; // either 3.1 or 3.1.1. as 3 or 4 respectively
  uint8_t                        pdp_cid; // PDP used by  mqtt client , range 1-16, default 1
                                          // TODO: THE REST OF THESE PARAMS
} qmtcfg_test_response;
