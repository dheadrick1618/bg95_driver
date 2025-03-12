#include "bg95_driver.h"

#include "at_cmd_cfun.h"
#include "at_cmd_cgact.h"
#include "at_cmd_cgpaddr.h"
#include "at_cmd_cops.h"
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

esp_err_t bg95_soft_restart(bg95_handle_t* handle)
{
  if (NULL == handle)
  {
    ESP_LOGE(TAG, "Invalid arg provided");
    return ESP_ERR_INVALID_ARG;
  }
  esp_err_t err;

  cfun_write_params_t write_params;
  memset(&write_params, 0, sizeof(cfun_write_params_t));

  write_params.fun_type = 0;

  err = at_cmd_handler_send_and_receive_cmd(
      &handle->at_handler, &AT_CMD_CFUN, AT_CMD_TYPE_WRITE, &write_params, NULL);
  if (err != ESP_OK)
  {
    return ESP_FAIL;
  }
  vTaskDelay(100);

  write_params.fun_type = 1;

  err = at_cmd_handler_send_and_receive_cmd(
      &handle->at_handler, &AT_CMD_CFUN, AT_CMD_TYPE_WRITE, &write_params, NULL);
  if (err != ESP_OK)
  {
    return ESP_FAIL;
  }
  vTaskDelay(100);

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

esp_err_t bg95_get_current_operator(bg95_handle_t* handle, cops_operator_data_t* operator_data)
{
  if (!handle || !operator_data || !handle->initialized)
  {
    ESP_LOGE(TAG, "Invalid arguments or handle not initialized");
    return ESP_ERR_INVALID_ARG;
  }

  // Create response structure
  cops_read_response_t cops_response = {0};

  // Send COPS read command and parse response
  esp_err_t err = at_cmd_handler_send_and_receive_cmd(&handle->at_handler,
                                                      &AT_CMD_COPS,
                                                      AT_CMD_TYPE_READ,
                                                      NULL, // No parameters needed for READ command
                                                      &cops_response);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to get operator information: %s", esp_err_to_name(err));
    return err;
  }

  // Transfer data from response to the operator_data output
  operator_data->mode = cops_response.mode;

  if (cops_response.present.has_format)
  {
    operator_data->format = cops_response.format;
  }
  else
  {
    // Default value if not provided
    operator_data->format = COPS_FORMAT_LONG_ALPHA;
  }

  if (cops_response.present.has_operator)
  {
    strncpy(operator_data->operator_name,
            cops_response.operator_name,
            sizeof(operator_data->operator_name) - 1);
    operator_data->operator_name[sizeof(operator_data->operator_name) - 1] =
        '\0'; // Ensure null termination
  }
  else
  {
    operator_data->operator_name[0] = '\0';
  }

  if (cops_response.present.has_act)
  {
    operator_data->act = cops_response.act;
  }
  else
  {
    // Default to GSM if not provided
    operator_data->act = COPS_ACT_GSM;
  }

  // Log the operator information
  if (operator_data->operator_name[0] != '\0')
  {
    ESP_LOGI(TAG,
             "Current operator: %s (Mode: %s, Format: %s, Act: %s)",
             operator_data->operator_name,
             enum_to_str(operator_data->mode, COPS_MODE_MAP, COPS_MODE_MAP_SIZE),
             enum_to_str(operator_data->format, COPS_FORMAT_MAP, COPS_FORMAT_MAP_SIZE),
             enum_to_str(operator_data->act, COPS_ACT_MAP, COPS_ACT_MAP_SIZE));
  }
  else
  {
    ESP_LOGI(TAG,
             "No operator selected (Mode: %s)",
             enum_to_str(operator_data->mode, COPS_MODE_MAP, COPS_MODE_MAP_SIZE));
  }

  return ESP_OK;
}

esp_err_t bg95_define_pdp_context(bg95_handle_t*     handle,
                                  uint8_t            cid,
                                  cgdcont_pdp_type_t pdp_type,
                                  const char*        apn)
{
  if (handle == NULL || apn == NULL || !handle->initialized)
  {
    ESP_LOGE(TAG, "Invalid arguments or handle not initialized");
    return ESP_ERR_INVALID_ARG;
  }

  /* Validate CID range (1-15 as per specification) */
  if (cid < 1 || cid > 15)
  {
    ESP_LOGE(TAG, "Invalid CID: %d (must be 1-15)", cid);
    return ESP_ERR_INVALID_ARG;
  }

  /* Create write parameters with required fields */
  cgdcont_write_params_t write_params;
  memset(&write_params, 0, sizeof(cgdcont_write_params_t));

  /* Set the fields and present flags */
  write_params.cid      = cid;
  write_params.pdp_type = pdp_type;
  strncpy(write_params.apn, apn, sizeof(write_params.apn) - 1);
  write_params.apn[sizeof(write_params.apn) - 1] = '\0';

  /* Set present flags */
  write_params.present.has_cid      = true;
  write_params.present.has_pdp_type = true;
  write_params.present.has_apn      = true;

  /* Send the command */
  ESP_LOGI(
      TAG, "Defining PDP context: CID=%d, PDP_TYPE=%d, APN=%s", cid, pdp_type, write_params.apn);

  esp_err_t err = at_cmd_handler_send_and_receive_cmd(
      &handle->at_handler, &AT_CMD_CGDCONT, AT_CMD_TYPE_WRITE, &write_params, NULL);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to define PDP context: %s", esp_err_to_name(err));
    return err;
  }

  ESP_LOGI(TAG, "Successfully defined PDP context (CID=%d, APN=%s)", cid, apn);
  return ESP_OK;
}

esp_err_t bg95_activate_pdp_context(bg95_handle_t* handle, const int cid)
{

  if (handle == NULL || !handle->initialized)
  {
    ESP_LOGE(TAG, "Invalid arguments or handle not initialized");
    return ESP_ERR_INVALID_ARG;
  }

  cgact_write_params_t write_params;
  memset(&write_params, 0, sizeof(cgact_write_params_t));

  write_params.cid   = cid;
  write_params.state = CGACT_STATE_ACTIVATED;

  return at_cmd_handler_send_and_receive_cmd(
      &handle->at_handler, &AT_CMD_CGACT, AT_CMD_TYPE_WRITE, &write_params, NULL);
}

esp_err_t bg95_is_pdp_context_active(bg95_handle_t* handle, uint8_t cid, bool* is_active)
{
  if (NULL == handle || NULL == is_active)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  // Initialize output
  *is_active = false;

  // Create response structure
  cgact_read_response_t response = {0};

  // Send CGACT read command
  esp_err_t err = at_cmd_handler_send_and_receive_cmd(
      &handle->at_handler, &AT_CMD_CGACT, AT_CMD_TYPE_READ, NULL, &response);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to get PDP context activation status: %s", esp_err_to_name(err));
    return err;
  }

  // Search for the specified CID
  for (int i = 0; i < response.num_contexts; i++)
  {
    if (response.contexts[i].cid == cid)
    {
      *is_active = (response.contexts[i].state == CGACT_STATE_ACTIVATED);
      ESP_LOGI(TAG, "PDP context %d is %s", cid, *is_active ? "active" : "inactive");
      return ESP_OK;
    }
  }

  // CID not found in response
  ESP_LOGW(TAG, "PDP context %d not found in CGACT response", cid);
  return ESP_ERR_NOT_FOUND;
}

esp_err_t
bg95_get_pdp_address_for_cid(bg95_handle_t* handle, uint8_t cid, char* address, size_t address_size)
{
  if (NULL == handle || NULL == address || address_size == 0)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  // Validate CID range
  if (cid < CGPADDR_CID_RANGE_MIN_VALUE || cid > CGPADDR_CID_RANGE_MAX_VALUE)
  {
    ESP_LOGE(TAG, "Invalid CID: %d (must be 1-15)", cid);
    return ESP_ERR_INVALID_ARG;
  }

  // Initialize output
  address[0] = '\0';

  // Create command parameters
  cgpaddr_write_params_t params = {.cid = cid};

  // Create response structure
  cgpaddr_write_response_t response = {0};

  // Send CGPADDR write command
  esp_err_t err = at_cmd_handler_send_and_receive_cmd(
      &handle->at_handler, &AT_CMD_CGPADDR, AT_CMD_TYPE_WRITE, &params, &response);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to get PDP address for CID %d: %s", cid, esp_err_to_name(err));
    return err;
  }

  // Verify that the CID in response matches requested CID
  if (response.cid != cid)
  {
    ESP_LOGE(TAG, "Response CID (%d) doesn't match requested CID (%d)", response.cid, cid);
    return ESP_ERR_INVALID_RESPONSE;
  }

  // Check if an address was returned
  if (response.address[0] == '\0')
  {
    ESP_LOGW(TAG, "No PDP address available for CID %d", cid);
    return ESP_ERR_NOT_FOUND;
  }

  // Copy address to output buffer
  if (strlen(response.address) >= address_size)
  {
    ESP_LOGW(TAG, "Address buffer too small, truncating result");
  }

  strncpy(address, response.address, address_size - 1);
  address[address_size - 1] = '\0'; // Ensure null termination

  ESP_LOGI(TAG, "PDP address for CID %d: %s", cid, address);
  return ESP_OK;
}

esp_err_t bg95_connect_to_network(bg95_handle_t* handle)
{
  esp_err_t err;
  // Check SIM card status (AT+CPIN)
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

  // Soft restart needed for setting to take place (AT+CFUN=0, AT+CFUN=1)
  // -----------------------------------------------------------

  // Activate PDP context (AT+CGACT)
  // -----------------------------------------------------------

  // Verify IP address assignment (AT+CGPADDR)
  // -----------------------------------------------------------

  // Check network registration status (AT+CEREG)  (CREG is for 2G)
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
