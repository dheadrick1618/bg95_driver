#include "bg95_driver.h"

#include "at_cmd_csq.h"
#include "at_cmd_handler.h"
#include "at_cmd_structure.h"

#include <esp_err.h>
#include <esp_log.h>
#include <stdio.h>
#include <string.h> // for memset

static const char* TAG = "BG95_DRIVER";

// Use a GPIO to send a pulse to the PWR KEY pin to turn on the BG95 module
// TODO: This
// static esp_err_t pulse_pwr_key_pin(void) {
//   return ESP_OK;
// }

esp_err_t bg95_init(bg95_handle_t* handle, bg95_uart_interface_t* uart)
{
  if (!handle || !uart)
  {
    return ESP_ERR_INVALID_ARG;
  }

  memset(handle, 0, sizeof(bg95_handle_t));

  // TODO: Power Cycle the BG95  -  then wait a bit to let it power up - do this using the PWRKEY

  // Initialize AT command handler with UART interface
  esp_err_t err = at_cmd_handler_init(&handle->at_handler, uart);
  if (err != ESP_OK)
  {
    return err;
  }

  handle->initialized = true;
  return ESP_OK;
}

esp_err_t bg95_deinit(bg95_handle_t* handle)
{
  // free pointer for bg95  handle
  if (NULL == handle)
  {
    ESP_LOGE(TAG, "Driver handle deinit failed");
    return ESP_ERR_INVALID_ARG;
  }
  free(handle);
  return ESP_OK;
}

esp_err_t bg95_get_sim_card_status(bg95_handle_t* handle, cpin_status_t* cpin_status)
{
  if (NULL == handle || NULL == cpin_status)
  {
    ESP_LOGE(TAG, "Invalid arg provided");
    return ESP_ERR_INVALID_ARG;
  }

  return at_cmd_handler_send_and_receive_cmd(
      &handle->at_handler, &AT_CMD_CPIN, AT_CMD_TYPE_READ, NULL, cpin_status);
}

esp_err_t bg95_get_signal_quality_dbm(bg95_handle_t* handle, int16_t* rssi_dbm)
{
  if (!handle || !rssi_dbm || !handle->initialized)
  {
    return ESP_ERR_INVALID_ARG;
  }

  // Create response structure
  csq_execute_response_t csq_response = {0};

  // Send CSQ command and parse response
  esp_err_t err =
      at_cmd_handler_send_and_receive_cmd(&handle->at_handler,
                                          &AT_CMD_CSQ,
                                          AT_CMD_TYPE_EXECUTE,
                                          NULL, // No parameters needed for EXECUTE command
                                          &csq_response);

  if (err != ESP_OK)
  {
    ESP_LOGE("BG95_SIGNAL", "Failed to get signal quality: %s", esp_err_to_name(err));
    return err;
  }

  // Convert RSSI to dBm
  *rssi_dbm = csq_rssi_to_dbm(csq_response.rssi);

  // // If ber pointer is provided, fill it too
  // if (ber != NULL)
  // {
  //   *ber = csq_response.ber;
  // }
  //
  // Log the signal quality
  // ESP_LOGI("BG95_SIGNAL",
  //          "Signal Quality: RSSI=%d (%d dBm), BER=%d (%s)",
  //          csq_response.rssi,
  //          *rssi_dbm,
  //          csq_response.ber,
  //          enum_to_str(csq_response.ber, CSQ_BER_MAP, CSQ_BER_MAP_SIZE));

  ESP_LOGI("BG95_SIGNAL", "Signal Quality: RSSI=%d (%d dBm)", csq_response.rssi, *rssi_dbm);

  // Check if the signal quality is known
  if (csq_response.rssi == CSQ_RSSI_UNKNOWN)
  {
    ESP_LOGW("BG95_SIGNAL", "Signal quality unknown");
    return ESP_ERR_NOT_FOUND; // Special error to indicate unknown signal
  }

  return ESP_OK;
}

esp_err_t bg95_connect_to_network(bg95_handle_t* handle)
{
  esp_err_t err;
  // Check SIM card status
  // -----------------------------------------------------------
  cpin_status_t sim_card_status;
  err = bg95_get_sim_card_status(handle, &sim_card_status);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to get PIN status: %d", err);
    return ESP_FAIL;
  }

  // Check signal quality (AT+CSQ)
  // -----------------------------------------------------------

  // Check available operators list (AT+COPS)
  // -----------------------------------------------------------

  // Define PDP context with your carriers APN (AT+CGDCONT)
  // -----------------------------------------------------------

  // Activate PDP context (AT+CGACT)
  // -----------------------------------------------------------

  // Verify IP address assignment (AT+CGPADDR)
  // -----------------------------------------------------------

  // Check network registration status (AT+CREG)
  // -----------------------------------------------------------

  return ESP_OK;
}

// esp_err_t bg95_get_available_operators(bg95_handle_t* handle, cops_test_response_t* operators)
// {
//   if (!handle || !operators || !handle->initialized)
//   {
//     ESP_LOGE(TAG, "Invalid arguments or handle not initialized");
//     return ESP_ERR_INVALID_ARG;
//   }
//
//   ESP_LOGI(TAG, "Creating response structure");
//   at_cmd_response_t response = {.parsed_data  = operators,
//                                 .raw_response = NULL,
//                                 .response_len = 0,
//                                 .cmd_type     = AT_CMD_TYPE_TEST,
//                                 .status       = ESP_OK};
//
//   ESP_LOGI(TAG, "Sending command to handler");
//   esp_err_t err = at_cmd_handler_send_and_receive_cmd(&handle->at_handler,
//                                                       &AT_CMD_COPS,
//                                                       AT_CMD_TYPE_TEST,
//                                                       NULL, // No params for TEST command
//                                                       &response);
//
//   ESP_LOGI(TAG, "Command handler returned: %d", err);
//   return err;
// }
//
// //-------------------------------------------------------------------------------------------------
// //-------------------------------------------------------------------------------------------------
//
// esp_err_t bg95_get_pin_status(bg95_handle_t* handle, cpin_read_response_t* status)
// {
//   if (!handle || !status || !handle->initialized)
//   {
//     return ESP_ERR_INVALID_ARG;
//   }
//
//   at_cmd_response_t response = {.parsed_data = status};
//
//   return at_cmd_handler_send_and_receive_cmd(
//       &handle->at_handler, &AT_CMD_CPIN, AT_CMD_TYPE_READ, NULL, &response);
// }
//
// //-------------------------------------------------------------------------------------------------
// //-------------------------------------------------------------------------------------------------
//
// esp_err_t bg95_get_network_registration(bg95_handle_t* handle, creg_read_response_t* status)
// {
//   if (!handle || !status || !handle->initialized)
//   {
//     return ESP_ERR_INVALID_ARG;
//   }
//
//   at_cmd_response_t response = {.parsed_data = status};
//
//   return at_cmd_handler_send_and_receive_cmd(
//       &handle->at_handler, &AT_CMD_CREG, AT_CMD_TYPE_READ, NULL, &response);
// }
//
// // TODO: this
// //  esp_err_t bg95_set_network_registration_mode(bg95_handle_t* handle, uint8_t mode) {
// //      if (!handle || !handle->initialized || mode > 2) {
// //          return ESP_ERR_INVALID_ARG;
// //      }
// //
// //      creg_write_params_t params = {
// //          .n = mode
// //      };
// //
// //      at_cmd_response_t response = {0};
// //
// //      return at_cmd_handler_send_and_receive_cmd(
// //          &handle->at_handler,
// //          &AT_CMD_CREG,
// //          AT_CMD_TYPE_WRITE,
// //          &params,
// //          &response
// //      );
// //  }
//
// esp_err_t bg95_get_supported_registration_modes(bg95_handle_t*        handle,
//                                                 creg_test_response_t* supported_modes)
// {
//   if (!handle || !supported_modes || !handle->initialized)
//   {
//     return ESP_ERR_INVALID_ARG;
//   }
//
//   at_cmd_response_t response = {.parsed_data = supported_modes};
//
//   return at_cmd_handler_send_and_receive_cmd(
//       &handle->at_handler, &AT_CMD_CREG, AT_CMD_TYPE_TEST, NULL, &response);
// }
//
// //-------------------------------------------------------------------------------------------------
// //-------------------------------------------------------------------------------------------------
//
// esp_err_t bg95_get_signal_quality(bg95_handle_t* handle, csq_response_t* signal_quality)
// {
//   if (!handle || !signal_quality || !handle->initialized)
//   {
//     return ESP_ERR_INVALID_ARG;
//   }
//
//   at_cmd_response_t response = {.parsed_data = signal_quality};
//
//   return at_cmd_handler_send_and_receive_cmd(
//       &handle->at_handler, &AT_CMD_CSQ, AT_CMD_TYPE_EXECUTE, NULL, &response);
// }
//
// esp_err_t bg95_get_supported_signal_quality_values(bg95_handle_t*       handle,
//                                                    csq_test_response_t* supported_values)
// {
//   if (!handle || !supported_values || !handle->initialized)
//   {
//     return ESP_ERR_INVALID_ARG;
//   }
//
//   at_cmd_response_t response = {.parsed_data = supported_values};
//
//   return at_cmd_handler_send_and_receive_cmd(
//       &handle->at_handler, &AT_CMD_CSQ, AT_CMD_TYPE_TEST, NULL, &response);
// }
//
// //-------------------------------------------------------------------------------------------------
// //-------------------------------------------------------------------------------------------------
//
// esp_err_t bg95_get_ps_attached_state(bg95_handle_t* handle, cgatt_read_params_t* state)
// {
//   if ((NULL == handle) || (NULL == state))
//   {
//     return ESP_ERR_INVALID_ARG;
//   }
//
//   at_cmd_response_t response = {.parsed_data = state};
//
//   return at_cmd_handler_send_and_receive_cmd(
//       &handle->at_handler, &AT_CMD_CGATT, AT_CMD_TYPE_READ, NULL, &response);
// }
//
// esp_err_t bg95_mqtt_network_conn_get_status(bg95_handle_t*           handle,
//                                             qmtopen_read_response_t* network_conn_response)
// {
//   if ((NULL == handle) || (NULL == network_conn_response))
//   {
//     return ESP_ERR_INVALID_ARG;
//   }
//
//   at_cmd_response_t response = {.parsed_data = network_conn_response};
//
//   return at_cmd_handler_send_and_receive_cmd(
//       &handle->at_handler, &AT_CMD_QMTOPEN, AT_CMD_TYPE_READ, NULL, &response);
// }
//
// esp_err_t bg95_mqtt_open_network_conn(bg95_handle_t*          handle,
//                                       qmtopen_write_params_t* mqtt_conn_params,
//                                       at_cmd_response_t*      mqtt_response_to_conn)
// { // pass client id, broker url, and port num
//
//   if ((NULL == handle) || (NULL == mqtt_conn_params))
//   {
//     return ESP_ERR_INVALID_ARG;
//   }
//
//   ESP_LOGI(TAG,
//            "MQTT Write params BEFORE call: client_id=%u, host=%s, port=%u",
//            mqtt_conn_params->client_id,
//            mqtt_conn_params->host_name,
//            mqtt_conn_params->port);
//
//   at_cmd_response_t response = {.parsed_data = mqtt_response_to_conn};
//
//   esp_err_t err = at_cmd_handler_send_and_receive_cmd(
//       &handle->at_handler, &AT_CMD_QMTOPEN, AT_CMD_TYPE_WRITE, &mqtt_conn_params, &response);
//
//   if (err != ESP_OK || mqtt_response_to_conn->status != ESP_OK)
//   {
//     ESP_LOGE(TAG, "Failed to open mqtt network conn: %d", err);
//     return err;
//   }
//   return ESP_OK;
// }
