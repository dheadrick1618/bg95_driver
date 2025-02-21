#pragma once
#include "at_cmd_cops.h"
#include "at_cmd_cpin.h"
#include "at_cmd_handler.h"
// #include "at_cmds.h"
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

// HIGH LEVEL fxn called by user - this calls a sequence of AT CMDS to connect to network bearer
esp_err_t bg95_connect_to_network(bg95_handle_t* handle);

//    =========  COMMAND SPECIFIC USER EXPOSED FXNS (API)  ==========   //
// =======================================================================

// -------------------- SIM RELATED CMDS ---------------------------
// CPIN - Enter PIN
esp_err_t bg95_get_sim_card_status(bg95_handle_t* handle, cpin_status_t* cpin_status);
// esp_err_t bg95_enter_pin(bg95_handle_t* handle, const char* pin);

// -------------------- NETWORK SERVICE CMDS ---------------------------
// CREG - Network Registration Status
// esp_err_t bg95_get_network_registration(bg95_handle_t* handle, creg_read_response_t* status);
// // esp_err_t bg95_set_network_registration_mode(bg95_handle_t* handle, uint8_t mode);
// esp_err_t bg95_get_supported_registration_modes(bg95_handle_t*        handle,
//                                                 creg_test_response_t* supported_modes);
//
// // COPS - Operator Selector
/**
 * @brief Get the current selected network operator
 *
 * @param handle Handle to the BG95 driver instance
 * @param operator_data Pointer to structure where operator data will be stored
 * @return esp_err_t ESP_OK on success, appropriate error code otherwise
 */
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
// // CGATT  - PS attach or detach
// esp_err_t bg95_attach_to_ps_domain(bg95_handle_t* handle); // no other args needed, these
// just send
//                                                            // write cmd with state 0 or state
//                                                            1
// esp_err_t bg95_detach_from_ps_domain(bg95_handle_t* handle);
// esp_err_t bg95_get_ps_attached_state(bg95_handle_t* handle, cgatt_read_params_t* state);
//
// // ---------------------------- MQTT CMDS ----------------------------------
// // QMTOPEN
// esp_err_t bg95_mqtt_network_conn_get_status(bg95_handle_t*           handle,
//                                             qmtopen_read_response_t* network_conn_response);
// esp_err_t bg95_mqtt_open_network_conn(bg95_handle_t*          handle,
//                                       qmtopen_write_params_t* mqtt_conn_params,
//                                       at_cmd_response_t*      mqtt_response_to_conn);
