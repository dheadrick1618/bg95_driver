#pragma once
#include "at_cmd_cgdcont.h"
#include "at_cmd_cops.h"
#include "at_cmd_cpin.h"
#include "at_cmd_handler.h"
#include "at_cmd_qmtcfg.h"
// #include "at_cmds.h"
#include "at_cmd_qmtopen.h"
#include "bg95_uart_interface.h"

#include <esp_err.h>

// Driver handle containing all context needed
typedef struct
{
  at_cmd_handler_t at_handler;
  bool             initialized;
} bg95_handle_t;

// Init a driver handle - scope of the handle pointer is responsibility of user
// The driver can be init with either a mock or hardware (actual) UART interface
esp_err_t bg95_init(bg95_handle_t* handle, bg95_uart_interface_t* uart);

// free handle pointer
esp_err_t bg95_deinit(bg95_handle_t* handle);

// As per the hardware design guide v1.6, the module can be turned on by driving the PWRKEY pin low
// for 500-1000ms
// TODO: Implement this
esp_err_t bg95_turn_module_on(bg95_handle_t* handle);

// TODO: this
//  AT+QPOWD  cmd  can be used to turn module off
esp_err_t bg95_turn_module_off_using_at_cmd(bg95_handle_t* handle);

// TODO: THIS
//  The module should attempt to be turned off typically with the AT cmd, but if its not responding,
//  holding the PWRKEY pin low for 650 - 1500ms then releasing it, will put the module in power-down
//  procedure.
esp_err_t bg95_turn_module_off_using_pwrkey(bg95_handle_t* handle);

// call CFUN 0 , then CFUN 1,  this is typically for settings to take place
esp_err_t bg95_soft_restart(bg95_handle_t* handle);

// HIGH LEVEL fxn called by user - this calls a sequence of AT CMDS to connect to network bearer
esp_err_t bg95_connect_to_network(bg95_handle_t* handle);

//    =========  COMMAND SPECIFIC USER EXPOSED FXNS (API)  ==========   //
// =======================================================================

// -------------------- SIM RELATED CMDS ---------------------------
// CPIN - Enter PIN
esp_err_t bg95_get_sim_card_status(bg95_handle_t* handle, cpin_status_t* cpin_status);
// esp_err_t bg95_enter_pin(bg95_handle_t* handle, const char* pin);

// -------------------- NETWORK SERVICE CMDS ---------------------------
// // COPS - Operator Selector
esp_err_t bg95_get_current_operator(bg95_handle_t* handle, cops_operator_data_t* operator_data);
// esp_err_t bg95_get_available_operators(bg95_handle_t* handle, cops_test_response_t*
// operators);
// // esp_err_t bg95_get_current_operator(bg95_handle_t* handle, cops_read_response_t*
// operator_info);
// // esp_err_t bg95_select_operator_manual(bg95_handle_t* handle, const cops_write_params_t*
// params);
// // esp_err_t bg95_select_operator_auto(bg95_handle_t* handle);
//
// CSQ - Signal Quality Report
esp_err_t bg95_get_signal_quality_dbm(bg95_handle_t* handle, int16_t* rssi_dbm);
// esp_err_t bg95_get_signal_quality(bg95_handle_t* handle, csq_response_t* signal_quality);
// esp_err_t bg95_get_supported_signal_quality_values(bg95_handle_t*       handle,
//                                                    csq_test_response_t* supported_values);
//
// // ---------------------- PACKET DOMAIN CMDS -----------------------------
// CGDCONT - Define PDP context
// Typically only the PDP type and the APN are provided by the user. Use the 'extended' version of
// the fxn if other paramaters must be defined
esp_err_t bg95_define_pdp_context(bg95_handle_t*     handle,
                                  uint8_t            cid,
                                  cgdcont_pdp_type_t pdp_type,
                                  const char*        apn);

esp_err_t bg95_define_pdp_context_extended(bg95_handle_t*               handle,
                                           const cgdcont_pdp_context_t* pdp_context);

esp_err_t bg95_activate_pdp_context(bg95_handle_t* handle, const int cid);

esp_err_t bg95_is_pdp_context_active(bg95_handle_t* handle, uint8_t cid, bool* is_active);

esp_err_t bg95_get_pdp_address_for_cid(bg95_handle_t* handle,
                                       uint8_t        cid,
                                       char*          address,
                                       size_t         address_size);

// // ---------------------------- MQTT CMDS ----------------------------------
// QMTCFG - config optional MQTT params
esp_err_t bg95_get_mqtt_config_params(bg95_handle_t* handle, qmtcfg_test_response_t* config_params);

esp_err_t bg95_mqtt_config_set_version(bg95_handle_t*                   handle,
                                       uint8_t                          client_idx,
                                       qmtcfg_version_t                 version,
                                       qmtcfg_write_version_response_t* response);

esp_err_t
bg95_mqtt_config_set_pdp_context(bg95_handle_t* handle, uint8_t client_idx, uint8_t pdp_cid);
esp_err_t bg95_mqtt_config_query_pdp_context(bg95_handle_t*                  handle,
                                             uint8_t                         client_idx,
                                             qmtcfg_write_pdpcid_response_t* response);

esp_err_t bg95_mqtt_config_set_ssl(bg95_handle_t*    handle,
                                   uint8_t           client_idx,
                                   qmtcfg_ssl_mode_t ssl_enable,
                                   uint8_t           ctx_index);
esp_err_t bg95_mqtt_config_query_ssl(bg95_handle_t*               handle,
                                     uint8_t                      client_idx,
                                     qmtcfg_write_ssl_response_t* response);

esp_err_t
bg95_mqtt_config_set_keepalive(bg95_handle_t* handle, uint8_t client_idx, uint16_t keep_alive_time);
esp_err_t bg95_mqtt_config_query_keepalive(bg95_handle_t*                     handle,
                                           uint8_t                            client_idx,
                                           qmtcfg_write_keepalive_response_t* response);

esp_err_t bg95_mqtt_config_set_session(bg95_handle_t*         handle,
                                       uint8_t                client_idx,
                                       qmtcfg_clean_session_t clean_session);
esp_err_t bg95_mqtt_config_query_session(bg95_handle_t*                   handle,
                                         uint8_t                          client_idx,
                                         qmtcfg_write_session_response_t* response);

esp_err_t bg95_mqtt_config_set_timeout(bg95_handle_t*          handle,
                                       uint8_t                 client_idx,
                                       uint8_t                 pkt_timeout,
                                       uint8_t                 retry_times,
                                       qmtcfg_timeout_notice_t timeout_notice);
esp_err_t bg95_mqtt_config_query_timeout(bg95_handle_t*                   handle,
                                         uint8_t                          client_idx,
                                         qmtcfg_write_timeout_response_t* response);

esp_err_t bg95_mqtt_config_set_will(bg95_handle_t*       handle,
                                    uint8_t              client_idx,
                                    qmtcfg_will_flag_t   will_flag,
                                    qmtcfg_will_qos_t    will_qos,
                                    qmtcfg_will_retain_t will_retain,
                                    const char*          will_topic,
                                    const char*          will_message);
esp_err_t bg95_mqtt_config_query_will(bg95_handle_t*                handle,
                                      uint8_t                       client_idx,
                                      qmtcfg_write_will_response_t* response);

esp_err_t bg95_mqtt_config_set_recv_mode(bg95_handle_t*          handle,
                                         uint8_t                 client_idx,
                                         qmtcfg_msg_recv_mode_t  msg_recv_mode,
                                         qmtcfg_msg_len_enable_t msg_len_enable);
esp_err_t bg95_mqtt_config_query_recv_mode(bg95_handle_t*                     handle,
                                           uint8_t                            client_idx,
                                           qmtcfg_write_recv_mode_response_t* response);

// // QMTOPEN
// Get current MQTT network connection status
esp_err_t bg95_mqtt_network_open_status(bg95_handle_t*           handle,
                                        uint8_t                  client_idx,
                                        qmtopen_read_response_t* status);

// Open a network connection for MQTT client
esp_err_t bg95_mqtt_open_network(bg95_handle_t*            handle,
                                 uint8_t                   client_idx,
                                 const char*               host_name,
                                 uint16_t                  port,
                                 qmtopen_write_response_t* response);

// Close a network connection for MQTT client
esp_err_t bg95_mqtt_close_network(bg95_handle_t* handle, uint8_t client_idx);
