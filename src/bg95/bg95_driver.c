#include "bg95_driver.h"

#include "at_cmd_cfun.h"
#include "at_cmd_cgact.h"
#include "at_cmd_cgdcont.h"
#include "at_cmd_cgpaddr.h"
#include "at_cmd_cops.h"
#include "at_cmd_csq.h"
#include "at_cmd_handler.h"
#include "at_cmd_qmtclose.h"
#include "at_cmd_qmtconn.h"
#include "at_cmd_qmtopen.h"
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
    ESP_LOGE(TAG, "Failed to get PIN status: %s", esp_err_to_name(err));
    return err;
  }

  // Check signal quality (AT+CSQ)
  // -----------------------------------------------------------
  int16_t rssi_dbm;
  err = bg95_get_signal_quality_dbm(handle, &rssi_dbm);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to check signal quality: %s", esp_err_to_name(err));
    return err;
  }

  // Check available operators list (AT+COPS)
  // -----------------------------------------------------------
  cops_operator_data_t operator_data;
  err = bg95_get_current_operator(handle, &operator_data);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to get current operator: %s", esp_err_to_name(err));
    return err;
  }

  // Define PDP context with your carriers APN (AT+CGDCONT)
  // -----------------------------------------------------------
  int                cid      = 1;
  cgdcont_pdp_type_t pdp_type = CGDCONT_PDP_TYPE_IPV4V6;
  const char*        apn      = "simbase"; // Use 'simbase' as the APN
  err                         = bg95_define_pdp_context(handle, cid, pdp_type, apn);
  if (err != ESP_OK)
  {
    ESP_LOGE(
        TAG, "Failed to define PDP context (set cid, pdp type, apn): %s", esp_err_to_name(err));
    return err;
  }

  // Soft restart needed for setting to take place (AT+CFUN=0, AT+CFUN=1)
  // -----------------------------------------------------------
  err = bg95_soft_restart(handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to soft restart BG95 %s", esp_err_to_name(err));
    return err;
  }

  // Activate PDP context (AT+CGACT)
  // -----------------------------------------------------------
  err = bg95_activate_pdp_context(handle, cid);
  if (err != ESP_OK)
  {
    ESP_LOGE(
        TAG, "Failed to activate PDP context for cid: %d, error: %s", cid, esp_err_to_name(err));
    return err;
  }

  // Verify IP address assignment (AT+CGPADDR)
  // -----------------------------------------------------------
  char ip_address[CGPADDR_ADDRESS_MAX_CHARS];
  err = bg95_get_pdp_address_for_cid(handle, cid, ip_address, sizeof(ip_address));
  if (err != ESP_OK)
  {

    ESP_LOGI(TAG, "Failed to get PDP context address: %s", esp_err_to_name(err));
    return err;
  }

  ESP_LOGI(TAG, "Successfully connected to network");
  return ESP_OK;
}

//////////////////////  MQTT  STUFF   /////////////////////////
// -----------------------------------------------------------
esp_err_t bg95_get_mqtt_config_params(bg95_handle_t* handle, qmtcfg_test_response_t* config_params)
{
  if (NULL == handle || NULL == config_params || !handle->initialized)
  {
    ESP_LOGE(TAG, "Invalid arguments or handle not initialized");
    return ESP_ERR_INVALID_ARG;
  }

  // TODO: Implement this - it should call query for each  of the config params one by one

  return ESP_OK;
}

esp_err_t
bg95_mqtt_config_set_pdp_context(bg95_handle_t* handle, uint8_t client_idx, uint8_t pdp_cid)
{
  if (NULL == handle || !handle->initialized)
  {
    ESP_LOGE(TAG, "Invalid handle or driver not initialized");
    return ESP_ERR_INVALID_ARG;
  }

  if (client_idx > 5)
  {
    ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-5)", client_idx);
    return ESP_ERR_INVALID_ARG;
  }

  if (pdp_cid < 1 || pdp_cid > 16)
  {
    ESP_LOGE(TAG, "Invalid PDP CID: %d (must be 1-16)", pdp_cid);
    return ESP_ERR_INVALID_ARG;
  }

  qmtcfg_write_params_t write_params             = {0};
  write_params.type                              = QMTCFG_TYPE_PDPCID;
  write_params.params.pdpcid.client_idx          = client_idx;
  write_params.params.pdpcid.pdp_cid             = pdp_cid;
  write_params.params.pdpcid.present.has_pdp_cid = true;

  esp_err_t err = at_cmd_handler_send_and_receive_cmd(
      &handle->at_handler, &AT_CMD_QMTCFG, AT_CMD_TYPE_WRITE, &write_params, NULL);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to set MQTT PDP context: %s", esp_err_to_name(err));
    return err;
  }

  ESP_LOGI(TAG, "Successfully set MQTT PDP context to %d for client %d", pdp_cid, client_idx);
  return ESP_OK;
}

esp_err_t bg95_mqtt_config_query_pdp_context(bg95_handle_t*                  handle,
                                             uint8_t                         client_idx,
                                             qmtcfg_write_pdpcid_response_t* response)
{
  if (NULL == handle || !handle->initialized || NULL == response)
  {
    ESP_LOGE(TAG, "Invalid handle, uninitialized driver, or NULL response pointer");
    return ESP_ERR_INVALID_ARG;
  }

  if (client_idx > 5)
  {
    ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-5)", client_idx);
    return ESP_ERR_INVALID_ARG;
  }

  qmtcfg_write_params_t write_params             = {0};
  write_params.type                              = QMTCFG_TYPE_PDPCID;
  write_params.params.pdpcid.client_idx          = client_idx;
  write_params.params.pdpcid.present.has_pdp_cid = false; // Query mode - no PDP CID provided

  qmtcfg_write_response_t write_response = {0};

  // Send query command
  esp_err_t err = at_cmd_handler_send_and_receive_cmd(
      &handle->at_handler, &AT_CMD_QMTCFG, AT_CMD_TYPE_WRITE, &write_params, &write_response);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to query MQTT PDP context: %s", esp_err_to_name(err));
    return err;
  }

  // Copy the relevant part of the response
  *response = write_response.response.pdpcid;

  // Log the result
  if (response->present.has_pdp_cid)
  {
    ESP_LOGI(TAG, "Current MQTT PDP context for client %d: %d", client_idx, response->pdp_cid);
  }
  else
  {
    ESP_LOGW(TAG, "PDP context information not available in response");
  }

  return ESP_OK;
}

// Set MQTT SSL mode and context index
esp_err_t bg95_mqtt_config_set_ssl(bg95_handle_t*    handle,
                                   uint8_t           client_idx,
                                   qmtcfg_ssl_mode_t ssl_enable,
                                   uint8_t           ctx_index)
{
  if (NULL == handle || !handle->initialized)
  {
    ESP_LOGE(TAG, "Invalid handle or driver not initialized");
    return ESP_ERR_INVALID_ARG;
  }

  if (client_idx > 5)
  {
    ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-5)", client_idx);
    return ESP_ERR_INVALID_ARG;
  }

  if (ssl_enable != QMTCFG_SSL_DISABLE && ssl_enable != QMTCFG_SSL_ENABLE)
  {
    ESP_LOGE(TAG, "Invalid SSL enable value: %d", ssl_enable);
    return ESP_ERR_INVALID_ARG;
  }

  if (ctx_index > 5)
  {
    ESP_LOGE(TAG, "Invalid SSL context index: %d (must be 0-5)", ctx_index);
    return ESP_ERR_INVALID_ARG;
  }

  qmtcfg_write_params_t write_params             = {0};
  write_params.type                              = QMTCFG_TYPE_SSL;
  write_params.params.ssl.client_idx             = client_idx;
  write_params.params.ssl.ssl_enable             = ssl_enable;
  write_params.params.ssl.ctx_index              = ctx_index;
  write_params.params.ssl.present.has_ssl_enable = true;
  write_params.params.ssl.present.has_ctx_index  = true;

  // Set command only expects OK response, no data
  esp_err_t err = at_cmd_handler_send_and_receive_cmd(&handle->at_handler,
                                                      &AT_CMD_QMTCFG,
                                                      AT_CMD_TYPE_WRITE,
                                                      &write_params,
                                                      NULL); // No response data needed

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to set MQTT SSL mode: %s", esp_err_to_name(err));
  }
  else
  {
    ESP_LOGI(TAG,
             "Successfully set MQTT SSL mode to %d with context %d for client %d",
             ssl_enable,
             ctx_index,
             client_idx);
  }

  return err;
}

// Query MQTT SSL configuration
esp_err_t bg95_mqtt_config_query_ssl(bg95_handle_t*               handle,
                                     uint8_t                      client_idx,
                                     qmtcfg_write_ssl_response_t* response)
{
  if (NULL == handle || !handle->initialized || NULL == response)
  {
    ESP_LOGE(TAG, "Invalid handle, uninitialized driver, or NULL response pointer");
    return ESP_ERR_INVALID_ARG;
  }

  if (client_idx > 5)
  {
    ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-5)", client_idx);
    return ESP_ERR_INVALID_ARG;
  }

  qmtcfg_write_params_t write_params             = {0};
  write_params.type                              = QMTCFG_TYPE_SSL;
  write_params.params.ssl.client_idx             = client_idx;
  write_params.params.ssl.present.has_ssl_enable = false; // Query mode
  write_params.params.ssl.present.has_ctx_index  = false; // Query mode

  qmtcfg_write_response_t write_response = {0};

  // Send query command
  esp_err_t err = at_cmd_handler_send_and_receive_cmd(
      &handle->at_handler, &AT_CMD_QMTCFG, AT_CMD_TYPE_WRITE, &write_params, &write_response);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to query MQTT SSL mode: %s", esp_err_to_name(err));
    return err;
  }

  // Copy the relevant part of the response
  *response = write_response.response.ssl;

  // Log the result
  if (response->present.has_ssl_enable)
  {
    ESP_LOGI(TAG,
             "MQTT SSL mode for client %d: %s",
             client_idx,
             response->ssl_enable == QMTCFG_SSL_ENABLE ? "Enabled" : "Disabled");

    if (response->present.has_ctx_index && response->ssl_enable == QMTCFG_SSL_ENABLE)
    {
      ESP_LOGI(TAG, "Using SSL context index: %d", response->ctx_index);
    }
  }
  else
  {
    ESP_LOGW(TAG, "SSL configuration information not available in response");
  }

  return ESP_OK;
}

// Set MQTT keepalive time
esp_err_t
bg95_mqtt_config_set_keepalive(bg95_handle_t* handle, uint8_t client_idx, uint16_t keep_alive_time)
{
  if (NULL == handle || !handle->initialized)
  {
    ESP_LOGE(TAG, "Invalid handle or driver not initialized");
    return ESP_ERR_INVALID_ARG;
  }

  if (client_idx > 5)
  {
    ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-5)", client_idx);
    return ESP_ERR_INVALID_ARG;
  }

  if (keep_alive_time > 3600) // Maximum allowed value per documentation
  {
    ESP_LOGE(TAG, "Invalid keep-alive time: %d (must be 0-3600)", keep_alive_time);
    return ESP_ERR_INVALID_ARG;
  }

  qmtcfg_write_params_t write_params                        = {0};
  write_params.type                                         = QMTCFG_TYPE_KEEPALIVE;
  write_params.params.keepalive.client_idx                  = client_idx;
  write_params.params.keepalive.keep_alive_time             = keep_alive_time;
  write_params.params.keepalive.present.has_keep_alive_time = true;

  // Set command only expects OK response, no data
  esp_err_t err = at_cmd_handler_send_and_receive_cmd(&handle->at_handler,
                                                      &AT_CMD_QMTCFG,
                                                      AT_CMD_TYPE_WRITE,
                                                      &write_params,
                                                      NULL); // No response data needed

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to set MQTT keep-alive time: %s", esp_err_to_name(err));
  }
  else
  {
    ESP_LOGI(TAG,
             "Successfully set MQTT keep-alive time to %d seconds for client %d",
             keep_alive_time,
             client_idx);
  }

  return err;
}

// Query MQTT keepalive configuration
esp_err_t bg95_mqtt_config_query_keepalive(bg95_handle_t*                     handle,
                                           uint8_t                            client_idx,
                                           qmtcfg_write_keepalive_response_t* response)
{
  if (NULL == handle || !handle->initialized || NULL == response)
  {
    ESP_LOGE(TAG, "Invalid handle, uninitialized driver, or NULL response pointer");
    return ESP_ERR_INVALID_ARG;
  }

  if (client_idx > 5)
  {
    ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-5)", client_idx);
    return ESP_ERR_INVALID_ARG;
  }

  qmtcfg_write_params_t write_params                        = {0};
  write_params.type                                         = QMTCFG_TYPE_KEEPALIVE;
  write_params.params.keepalive.client_idx                  = client_idx;
  write_params.params.keepalive.present.has_keep_alive_time = false; // Query mode

  qmtcfg_write_response_t write_response = {0};

  // Send query command
  esp_err_t err = at_cmd_handler_send_and_receive_cmd(
      &handle->at_handler, &AT_CMD_QMTCFG, AT_CMD_TYPE_WRITE, &write_params, &write_response);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to query MQTT keep-alive time: %s", esp_err_to_name(err));
    return err;
  }

  // Copy the relevant part of the response
  *response = write_response.response.keepalive;

  // Log the result
  if (response->present.has_keep_alive_time)
  {
    ESP_LOGI(TAG,
             "MQTT keep-alive time for client %d: %d seconds",
             client_idx,
             response->keep_alive_time);
  }
  else
  {
    ESP_LOGW(TAG, "Keep-alive time information not available in response");
  }

  return ESP_OK;
}

// Set MQTT session type
esp_err_t bg95_mqtt_config_set_session(bg95_handle_t*         handle,
                                       uint8_t                client_idx,
                                       qmtcfg_clean_session_t clean_session)
{
  if (NULL == handle || !handle->initialized)
  {
    ESP_LOGE(TAG, "Invalid handle or driver not initialized");
    return ESP_ERR_INVALID_ARG;
  }

  if (client_idx > 5)
  {
    ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-5)", client_idx);
    return ESP_ERR_INVALID_ARG;
  }

  if (clean_session != QMTCFG_CLEAN_SESSION_DISABLE && clean_session != QMTCFG_CLEAN_SESSION_ENABLE)
  {
    ESP_LOGE(TAG, "Invalid clean session value: %d", clean_session);
    return ESP_ERR_INVALID_ARG;
  }

  qmtcfg_write_params_t write_params                    = {0};
  write_params.type                                     = QMTCFG_TYPE_SESSION;
  write_params.params.session.client_idx                = client_idx;
  write_params.params.session.clean_session             = clean_session;
  write_params.params.session.present.has_clean_session = true;

  // Set command only expects OK response, no data
  esp_err_t err = at_cmd_handler_send_and_receive_cmd(&handle->at_handler,
                                                      &AT_CMD_QMTCFG,
                                                      AT_CMD_TYPE_WRITE,
                                                      &write_params,
                                                      NULL); // No response data needed

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to set MQTT session type: %s", esp_err_to_name(err));
  }
  else
  {
    ESP_LOGI(TAG,
             "Successfully set MQTT session type to %s for client %d",
             clean_session == QMTCFG_CLEAN_SESSION_ENABLE ? "clean (discard previous info)"
                                                          : "persistent (store subscriptions)",
             client_idx);
  }

  return err;
}

// Query MQTT session configuration
esp_err_t bg95_mqtt_config_query_session(bg95_handle_t*                   handle,
                                         uint8_t                          client_idx,
                                         qmtcfg_write_session_response_t* response)
{
  if (NULL == handle || !handle->initialized || NULL == response)
  {
    ESP_LOGE(TAG, "Invalid handle, uninitialized driver, or NULL response pointer");
    return ESP_ERR_INVALID_ARG;
  }

  if (client_idx > 5)
  {
    ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-5)", client_idx);
    return ESP_ERR_INVALID_ARG;
  }

  qmtcfg_write_params_t write_params                    = {0};
  write_params.type                                     = QMTCFG_TYPE_SESSION;
  write_params.params.session.client_idx                = client_idx;
  write_params.params.session.present.has_clean_session = false; // Query mode

  qmtcfg_write_response_t write_response = {0};

  // Send query command
  esp_err_t err = at_cmd_handler_send_and_receive_cmd(
      &handle->at_handler, &AT_CMD_QMTCFG, AT_CMD_TYPE_WRITE, &write_params, &write_response);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to query MQTT session type: %s", esp_err_to_name(err));
    return err;
  }

  // Copy the relevant part of the response
  *response = write_response.response.session;

  // Log the result
  if (response->present.has_clean_session)
  {
    ESP_LOGI(TAG,
             "MQTT session type for client %d: %s",
             client_idx,
             response->clean_session == QMTCFG_CLEAN_SESSION_ENABLE
                 ? "Clean (discard information)"
                 : "Persistent (store subscriptions)");
  }
  else
  {
    ESP_LOGW(TAG, "Session type information not available in response");
  }

  return ESP_OK;
}

// Set MQTT timeout parameters
esp_err_t bg95_mqtt_config_set_timeout(bg95_handle_t*          handle,
                                       uint8_t                 client_idx,
                                       uint8_t                 pkt_timeout,
                                       uint8_t                 retry_times,
                                       qmtcfg_timeout_notice_t timeout_notice)
{
  if (NULL == handle || !handle->initialized)
  {
    ESP_LOGE(TAG, "Invalid handle or driver not initialized");
    return ESP_ERR_INVALID_ARG;
  }

  if (client_idx > 5)
  {
    ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-5)", client_idx);
    return ESP_ERR_INVALID_ARG;
  }

  if (pkt_timeout < 1 || pkt_timeout > 60)
  {
    ESP_LOGE(TAG, "Invalid packet timeout: %d (must be 1-60)", pkt_timeout);
    return ESP_ERR_INVALID_ARG;
  }

  if (retry_times > 10)
  {
    ESP_LOGE(TAG, "Invalid retry times: %d (must be 0-10)", retry_times);
    return ESP_ERR_INVALID_ARG;
  }

  if (timeout_notice != QMTCFG_TIMEOUT_NOTICE_DISABLE &&
      timeout_notice != QMTCFG_TIMEOUT_NOTICE_ENABLE)
  {
    ESP_LOGE(TAG, "Invalid timeout notice value: %d", timeout_notice);
    return ESP_ERR_INVALID_ARG;
  }

  qmtcfg_write_params_t write_params                     = {0};
  write_params.type                                      = QMTCFG_TYPE_TIMEOUT;
  write_params.params.timeout.client_idx                 = client_idx;
  write_params.params.timeout.pkt_timeout                = pkt_timeout;
  write_params.params.timeout.retry_times                = retry_times;
  write_params.params.timeout.timeout_notice             = timeout_notice;
  write_params.params.timeout.present.has_pkt_timeout    = true;
  write_params.params.timeout.present.has_retry_times    = true;
  write_params.params.timeout.present.has_timeout_notice = true;

  // Set command only expects OK response, no data
  esp_err_t err = at_cmd_handler_send_and_receive_cmd(&handle->at_handler,
                                                      &AT_CMD_QMTCFG,
                                                      AT_CMD_TYPE_WRITE,
                                                      &write_params,
                                                      NULL); // No response data needed

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to set MQTT timeout parameters: %s", esp_err_to_name(err));
  }
  else
  {
    ESP_LOGI(TAG,
             "Successfully set MQTT timeout parameters for client %d: "
             "packet timeout=%d, retry times=%d, timeout notice=%d",
             client_idx,
             pkt_timeout,
             retry_times,
             timeout_notice);
  }

  return err;
}

// Query MQTT timeout configuration
esp_err_t bg95_mqtt_config_query_timeout(bg95_handle_t*                   handle,
                                         uint8_t                          client_idx,
                                         qmtcfg_write_timeout_response_t* response)
{
  if (NULL == handle || !handle->initialized || NULL == response)
  {
    ESP_LOGE(TAG, "Invalid handle, uninitialized driver, or NULL response pointer");
    return ESP_ERR_INVALID_ARG;
  }

  if (client_idx > 5)
  {
    ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-5)", client_idx);
    return ESP_ERR_INVALID_ARG;
  }

  qmtcfg_write_params_t write_params     = {0};
  write_params.type                      = QMTCFG_TYPE_TIMEOUT;
  write_params.params.timeout.client_idx = client_idx;
  // Set all present flags to false for query mode
  write_params.params.timeout.present.has_pkt_timeout    = false;
  write_params.params.timeout.present.has_retry_times    = false;
  write_params.params.timeout.present.has_timeout_notice = false;

  qmtcfg_write_response_t write_response = {0};

  // Send query command
  esp_err_t err = at_cmd_handler_send_and_receive_cmd(
      &handle->at_handler, &AT_CMD_QMTCFG, AT_CMD_TYPE_WRITE, &write_params, &write_response);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to query MQTT timeout parameters: %s", esp_err_to_name(err));
    return err;
  }

  // Copy the relevant part of the response
  *response = write_response.response.timeout;

  // Log the result
  ESP_LOGI(TAG, "MQTT timeout parameters for client %d:", client_idx);

  if (response->present.has_pkt_timeout)
  {
    ESP_LOGI(TAG, "  Packet timeout: %d seconds", response->pkt_timeout);
  }

  if (response->present.has_retry_times)
  {
    ESP_LOGI(TAG, "  Retry times: %d", response->retry_times);
  }

  if (response->present.has_timeout_notice)
  {
    ESP_LOGI(TAG,
             "  Timeout notice: %s",
             response->timeout_notice == QMTCFG_TIMEOUT_NOTICE_ENABLE ? "Enabled" : "Disabled");
  }

  return ESP_OK;
}

// Set MQTT will configuration
esp_err_t bg95_mqtt_config_set_will(bg95_handle_t*       handle,
                                    uint8_t              client_idx,
                                    qmtcfg_will_flag_t   will_flag,
                                    qmtcfg_will_qos_t    will_qos,
                                    qmtcfg_will_retain_t will_retain,
                                    const char*          will_topic,
                                    const char*          will_message)
{
  if (NULL == handle || !handle->initialized)
  {
    ESP_LOGE(TAG, "Invalid handle or driver not initialized");
    return ESP_ERR_INVALID_ARG;
  }

  if (client_idx > 5)
  {
    ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-5)", client_idx);
    return ESP_ERR_INVALID_ARG;
  }

  if (will_flag != QMTCFG_WILL_FLAG_IGNORE && will_flag != QMTCFG_WILL_FLAG_REQUIRE)
  {
    ESP_LOGE(TAG, "Invalid will flag value: %d", will_flag);
    return ESP_ERR_INVALID_ARG;
  }

  qmtcfg_write_params_t write_params             = {0};
  write_params.type                              = QMTCFG_TYPE_WILL;
  write_params.params.will.client_idx            = client_idx;
  write_params.params.will.will_flag             = will_flag;
  write_params.params.will.present.has_will_flag = true;

  // If will flag is REQUIRE, additional parameters are needed
  if (will_flag == QMTCFG_WILL_FLAG_REQUIRE)
  {
    if (will_qos > QMTCFG_WILL_QOS_2)
    {
      ESP_LOGE(TAG, "Invalid will QoS value: %d", will_qos);
      return ESP_ERR_INVALID_ARG;
    }

    if (will_retain != QMTCFG_WILL_RETAIN_DISABLE && will_retain != QMTCFG_WILL_RETAIN_ENABLE)
    {
      ESP_LOGE(TAG, "Invalid will retain value: %d", will_retain);
      return ESP_ERR_INVALID_ARG;
    }

    if (will_topic == NULL || will_message == NULL)
    {
      ESP_LOGE(TAG, "Will topic and message must be provided when will flag is REQUIRE");
      return ESP_ERR_INVALID_ARG;
    }

    write_params.params.will.will_qos    = will_qos;
    write_params.params.will.will_retain = will_retain;
    strncpy(write_params.params.will.will_topic,
            will_topic,
            sizeof(write_params.params.will.will_topic) - 1);
    strncpy(write_params.params.will.will_message,
            will_message,
            sizeof(write_params.params.will.will_message) - 1);

    write_params.params.will.present.has_will_qos     = true;
    write_params.params.will.present.has_will_retain  = true;
    write_params.params.will.present.has_will_topic   = true;
    write_params.params.will.present.has_will_message = true;
  }

  // Set command only expects OK response, no data
  esp_err_t err = at_cmd_handler_send_and_receive_cmd(&handle->at_handler,
                                                      &AT_CMD_QMTCFG,
                                                      AT_CMD_TYPE_WRITE,
                                                      &write_params,
                                                      NULL); // No response data needed

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to set MQTT will configuration: %s", esp_err_to_name(err));
  }
  else
  {
    if (will_flag == QMTCFG_WILL_FLAG_IGNORE)
    {
      ESP_LOGI(TAG, "Successfully disabled MQTT will for client %d", client_idx);
    }
    else
    {
      ESP_LOGI(TAG,
               "Successfully set MQTT will for client %d: QoS=%d, Retain=%d, "
               "Topic='%s', Message='%s'",
               client_idx,
               will_qos,
               will_retain,
               will_topic,
               will_message);
    }
  }

  return err;
}

// Query MQTT will configuration
esp_err_t bg95_mqtt_config_query_will(bg95_handle_t*                handle,
                                      uint8_t                       client_idx,
                                      qmtcfg_write_will_response_t* response)
{
  if (NULL == handle || !handle->initialized || NULL == response)
  {
    ESP_LOGE(TAG, "Invalid handle, uninitialized driver, or NULL response pointer");
    return ESP_ERR_INVALID_ARG;
  }

  if (client_idx > 5)
  {
    ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-5)", client_idx);
    return ESP_ERR_INVALID_ARG;
  }

  qmtcfg_write_params_t write_params             = {0};
  write_params.type                              = QMTCFG_TYPE_WILL;
  write_params.params.will.client_idx            = client_idx;
  write_params.params.will.present.has_will_flag = false; // Query mode

  qmtcfg_write_response_t write_response = {0};

  // Send query command
  esp_err_t err = at_cmd_handler_send_and_receive_cmd(
      &handle->at_handler, &AT_CMD_QMTCFG, AT_CMD_TYPE_WRITE, &write_params, &write_response);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to query MQTT will configuration: %s", esp_err_to_name(err));
    return err;
  }

  // Copy the relevant part of the response
  *response = write_response.response.will;

  // Log the result
  if (response->present.has_will_flag)
  {
    if (response->will_flag == QMTCFG_WILL_FLAG_IGNORE)
    {
      ESP_LOGI(TAG, "MQTT will is disabled for client %d", client_idx);
    }
    else
    {
      ESP_LOGI(TAG, "MQTT will is enabled for client %d", client_idx);

      if (response->present.has_will_qos)
      {
        ESP_LOGI(TAG, "  Will QoS: %d", response->will_qos);
      }

      if (response->present.has_will_retain)
      {
        ESP_LOGI(TAG,
                 "  Will retain: %s",
                 response->will_retain == QMTCFG_WILL_RETAIN_ENABLE ? "Yes" : "No");
      }

      if (response->present.has_will_topic)
      {
        ESP_LOGI(TAG, "  Will topic: %s", response->will_topic);
      }

      if (response->present.has_will_message)
      {
        ESP_LOGI(TAG, "  Will message: %s", response->will_message);
      }
    }
  }
  else
  {
    ESP_LOGW(TAG, "Will configuration information not available in response");
  }

  return ESP_OK;
}

// Set MQTT message receive mode
esp_err_t bg95_mqtt_config_set_recv_mode(bg95_handle_t*          handle,
                                         uint8_t                 client_idx,
                                         qmtcfg_msg_recv_mode_t  msg_recv_mode,
                                         qmtcfg_msg_len_enable_t msg_len_enable)
{
  if (NULL == handle || !handle->initialized)
  {
    ESP_LOGE(TAG, "Invalid handle or driver not initialized");
    return ESP_ERR_INVALID_ARG;
  }

  if (client_idx > 5)
  {
    ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-5)", client_idx);
    return ESP_ERR_INVALID_ARG;
  }

  if (msg_recv_mode != QMTCFG_MSG_RECV_MODE_CONTAIN_IN_URC &&
      msg_recv_mode != QMTCFG_MSG_RECV_MODE_NOT_CONTAIN_IN_URC)
  {
    ESP_LOGE(TAG, "Invalid message receive mode: %d", msg_recv_mode);
    return ESP_ERR_INVALID_ARG;
  }

  if (msg_len_enable != QMTCFG_MSG_LEN_DISABLE && msg_len_enable != QMTCFG_MSG_LEN_ENABLE)
  {
    ESP_LOGE(TAG, "Invalid message length enable value: %d", msg_len_enable);
    return ESP_ERR_INVALID_ARG;
  }

  qmtcfg_write_params_t write_params                       = {0};
  write_params.type                                        = QMTCFG_TYPE_RECV_MODE;
  write_params.params.recv_mode.client_idx                 = client_idx;
  write_params.params.recv_mode.msg_recv_mode              = msg_recv_mode;
  write_params.params.recv_mode.msg_len_enable             = msg_len_enable;
  write_params.params.recv_mode.present.has_msg_recv_mode  = true;
  write_params.params.recv_mode.present.has_msg_len_enable = true;

  // Set command only expects OK response, no data
  esp_err_t err = at_cmd_handler_send_and_receive_cmd(&handle->at_handler,
                                                      &AT_CMD_QMTCFG,
                                                      AT_CMD_TYPE_WRITE,
                                                      &write_params,
                                                      NULL); // No response data needed

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to set MQTT receive mode: %s", esp_err_to_name(err));
  }
  else
  {
    ESP_LOGI(TAG,
             "Successfully set MQTT receive mode for client %d: "
             "Message %s in URC, Length %s in URC",
             client_idx,
             msg_recv_mode == QMTCFG_MSG_RECV_MODE_CONTAIN_IN_URC ? "contained" : "not contained",
             msg_len_enable == QMTCFG_MSG_LEN_ENABLE ? "contained" : "not contained");
  }

  return err;
}

// Query MQTT receive mode configuration
esp_err_t bg95_mqtt_config_query_recv_mode(bg95_handle_t*                     handle,
                                           uint8_t                            client_idx,
                                           qmtcfg_write_recv_mode_response_t* response)
{
  if (NULL == handle || !handle->initialized || NULL == response)
  {
    ESP_LOGE(TAG, "Invalid handle, uninitialized driver, or NULL response pointer");
    return ESP_ERR_INVALID_ARG;
  }

  if (client_idx > 5)
  {
    ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-5)", client_idx);
    return ESP_ERR_INVALID_ARG;
  }

  qmtcfg_write_params_t write_params                       = {0};
  write_params.type                                        = QMTCFG_TYPE_RECV_MODE;
  write_params.params.recv_mode.client_idx                 = client_idx;
  write_params.params.recv_mode.present.has_msg_recv_mode  = false; // Query mode
  write_params.params.recv_mode.present.has_msg_len_enable = false; // Query mode

  qmtcfg_write_response_t write_response = {0};

  // Send query command
  esp_err_t err = at_cmd_handler_send_and_receive_cmd(
      &handle->at_handler, &AT_CMD_QMTCFG, AT_CMD_TYPE_WRITE, &write_params, &write_response);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to query MQTT receive mode: %s", esp_err_to_name(err));
    return err;
  }

  // Copy the relevant part of the response
  *response = write_response.response.recv_mode;

  // Log the result
  ESP_LOGI(TAG, "MQTT receive mode for client %d:", client_idx);

  if (response->present.has_msg_recv_mode)
  {
    ESP_LOGI(TAG,
             "  Message receive mode: %s",
             response->msg_recv_mode == QMTCFG_MSG_RECV_MODE_CONTAIN_IN_URC
                 ? "Contained in URC"
                 : "Not contained in URC");
  }
  else
  {
    ESP_LOGW(TAG, "  Message receive mode information not available");
  }

  if (response->present.has_msg_len_enable)
  {
    ESP_LOGI(TAG,
             "  Message length: %s",
             response->msg_len_enable == QMTCFG_MSG_LEN_ENABLE ? "Contained in URC"
                                                               : "Not contained in URC");
  }
  else
  {
    ESP_LOGW(TAG, "  Message length enable information not available");
  }

  return ESP_OK;
}

// Get current MQTT network connection status
esp_err_t bg95_mqtt_network_open_status(bg95_handle_t*           handle,
                                        uint8_t                  client_idx,
                                        qmtopen_read_response_t* status)
{
  if (NULL == handle || NULL == status || !handle->initialized)
  {
    ESP_LOGE(TAG, "Invalid arguments or handle not initialized");
    return ESP_ERR_INVALID_ARG;
  }

  if (client_idx > QMTOPEN_CLIENT_IDX_MAX)
  {
    ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-%d)", client_idx, QMTOPEN_CLIENT_IDX_MAX);
    return ESP_ERR_INVALID_ARG;
  }

  // Send read command (AT+QMTOPEN?) to get current connections
  esp_err_t err = at_cmd_handler_send_and_receive_cmd(
      &handle->at_handler, &AT_CMD_QMTOPEN, AT_CMD_TYPE_READ, NULL, status);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to get MQTT network connection status: %s", esp_err_to_name(err));
    return err;
  }

  // Check if we got info for the requested client_idx
  if (!status->present.has_client_idx || status->client_idx != client_idx)
  {
    ESP_LOGW(TAG, "No connection info found for MQTT client %d", client_idx);
    return ESP_ERR_NOT_FOUND;
  }

  ESP_LOGI(TAG,
           "MQTT client %d connected to %s:%d",
           status->client_idx,
           status->host_name,
           status->port);

  return ESP_OK;
}

// Open a network connection for MQTT client
esp_err_t bg95_mqtt_open_network(bg95_handle_t*            handle,
                                 uint8_t                   client_idx,
                                 const char*               host_name,
                                 uint16_t                  port,
                                 qmtopen_write_response_t* response)
{
  if (NULL == handle || NULL == host_name || !handle->initialized)
  {
    ESP_LOGE(TAG, "Invalid arguments or handle not initialized");
    return ESP_ERR_INVALID_ARG;
  }

  if (client_idx > QMTOPEN_CLIENT_IDX_MAX)
  {
    ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-%d)", client_idx, QMTOPEN_CLIENT_IDX_MAX);
    return ESP_ERR_INVALID_ARG;
  }

  if (port > QMTOPEN_PORT_MAX)
  {
    ESP_LOGE(TAG, "Invalid port: %d (must be 0-%d)", port, QMTOPEN_PORT_MAX);
    return ESP_ERR_INVALID_ARG;
  }

  if (strlen(host_name) > QMTOPEN_HOST_NAME_MAX_SIZE)
  {
    ESP_LOGE(TAG, "Host name too long (max %d chars)", QMTOPEN_HOST_NAME_MAX_SIZE);
    return ESP_ERR_INVALID_ARG;
  }

  // Prepare write parameters
  qmtopen_write_params_t params = {.client_idx = client_idx, .port = port};
  strncpy(params.host_name, host_name, sizeof(params.host_name) - 1);
  params.host_name[sizeof(params.host_name) - 1] = '\0'; // Ensure null termination

  ESP_LOGI(
      TAG, "Opening MQTT network connection for client %d to %s:%d", client_idx, host_name, port);

  // Send the command
  qmtopen_write_response_t local_response = {0};
  esp_err_t                err            = at_cmd_handler_send_and_receive_cmd(&handle->at_handler,
                                                      &AT_CMD_QMTOPEN,
                                                      AT_CMD_TYPE_WRITE,
                                                      &params,
                                                      response ? response : &local_response);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to send MQTT open network command: %s", esp_err_to_name(err));
    return err;
  }

  // If we got a result code in the immediate response, check if it was successful
  if ((response && response->present.has_result) ||
      (!response && local_response.present.has_result))
  {
    qmtopen_result_t result = response ? response->result : local_response.result;

    if (result != QMTOPEN_RESULT_OPEN_SUCCESS)
    {
      ESP_LOGE(TAG,
               "MQTT open network failed with result: %d (%s)",
               result,
               enum_to_str(result, QMTOPEN_RESULT_MAP, QMTOPEN_RESULT_MAP_SIZE));
      return ESP_FAIL;
    }

    ESP_LOGI(TAG, "MQTT network connection opened successfully for client %d", client_idx);
  }
  else
  {
    // If no result code in the immediate response, that's expected
    // The URC with the result will come later, the caller needs to wait for it
    ESP_LOGI(TAG, "MQTT open network command sent successfully, waiting for connection result");
  }

  return ESP_OK;
}

// Close a network connection for MQTT client
esp_err_t bg95_mqtt_close_network(bg95_handle_t*             handle,
                                  uint8_t                    client_idx,
                                  qmtclose_write_response_t* response)
{
  if (NULL == handle || !handle->initialized)
  {
    ESP_LOGE(TAG, "Invalid handle or handle not initialized");
    return ESP_ERR_INVALID_ARG;
  }

  if (client_idx > QMTCLOSE_CLIENT_IDX_MAX)
  {
    ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-%d)", client_idx, QMTCLOSE_CLIENT_IDX_MAX);
    return ESP_ERR_INVALID_ARG;
  }

  // Prepare write parameters
  qmtclose_write_params_t params = {.client_idx = client_idx};

  ESP_LOGI(TAG, "Closing MQTT network connection for client %d", client_idx);

  // Send the command
  qmtclose_write_response_t local_response = {0};
  esp_err_t                 err = at_cmd_handler_send_and_receive_cmd(&handle->at_handler,
                                                      &AT_CMD_QMTCLOSE,
                                                      AT_CMD_TYPE_WRITE,
                                                      &params,
                                                      response ? response : &local_response);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to send MQTT close network command: %s", esp_err_to_name(err));
    return err;
  }

  // If we got a result code in the immediate response, check if it was successful
  if ((response && response->present.has_result) ||
      (!response && local_response.present.has_result))
  {
    qmtclose_result_t result = response ? response->result : local_response.result;

    if (result != QMTCLOSE_RESULT_CLOSE_SUCCESS)
    {
      ESP_LOGE(TAG,
               "MQTT close network failed with result: %d (%s)",
               result,
               enum_to_str(result, QMTCLOSE_RESULT_MAP, QMTCLOSE_RESULT_MAP_SIZE));
      return ESP_FAIL;
    }

    ESP_LOGI(TAG, "MQTT network connection closed successfully for client %d", client_idx);
  }
  else
  {
    // If no result code in the immediate response, that's expected
    // The URC with the result will come later, the caller needs to wait for it
    ESP_LOGI(TAG, "MQTT close network command sent successfully, waiting for result");
  }

  return ESP_OK;
}

// Check the current connection state of an MQTT client
esp_err_t bg95_mqtt_query_connection_state(bg95_handle_t*           handle,
                                           uint8_t                  client_idx,
                                           qmtconn_read_response_t* status)
{
  if (NULL == handle || NULL == status || !handle->initialized)
  {
    ESP_LOGE(TAG, "Invalid arguments or handle not initialized");
    return ESP_ERR_INVALID_ARG;
  }

  if (client_idx > QMTCONN_CLIENT_IDX_MAX)
  {
    ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-%d)", client_idx, QMTCONN_CLIENT_IDX_MAX);
    return ESP_ERR_INVALID_ARG;
  }

  // Send read command (AT+QMTCONN?) to get current connection state
  esp_err_t err = at_cmd_handler_send_and_receive_cmd(
      &handle->at_handler, &AT_CMD_QMTCONN, AT_CMD_TYPE_READ, NULL, status);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to get MQTT connection status: %s", esp_err_to_name(err));
    return err;
  }

  // If no status returned but command succeeded, this means no client is connected
  if (!status->present.has_client_idx || status->client_idx != client_idx)
  {
    ESP_LOGD(TAG, "No connection information found for MQTT client %d", client_idx);
    return ESP_ERR_NOT_FOUND;
  }

  ESP_LOGI(TAG,
           "MQTT client %d connection state: %d (%s)",
           client_idx,
           status->state,
           enum_to_str(status->state, QMTCONN_STATE_MAP, QMTCONN_STATE_MAP_SIZE));

  return ESP_OK;
}

esp_err_t bg95_mqtt_connect(bg95_handle_t*            handle,
                            uint8_t                   client_idx,
                            const char*               client_id_str,
                            const char*               username,
                            const char*               password,
                            qmtconn_write_response_t* response)
{
  if (NULL == handle || NULL == client_id_str || !handle->initialized)
  {
    ESP_LOGE(TAG, "Invalid arguments or handle not initialized");
    return ESP_ERR_INVALID_ARG;
  }

  if (client_idx > QMTCONN_CLIENT_IDX_MAX)
  {
    ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-%d)", client_idx, QMTCONN_CLIENT_IDX_MAX);
    return ESP_ERR_INVALID_ARG;
  }

  if (strlen(client_id_str) > QMTCONN_CLIENT_ID_MAX_SIZE)
  {
    ESP_LOGE(TAG, "Client ID too long (max %d chars)", QMTCONN_CLIENT_ID_MAX_SIZE);
    return ESP_ERR_INVALID_ARG;
  }

  if (username != NULL && strlen(username) > QMTCONN_USERNAME_MAX_SIZE)
  {
    ESP_LOGE(TAG, "Username too long (max %d chars)", QMTCONN_USERNAME_MAX_SIZE);
    return ESP_ERR_INVALID_ARG;
  }

  if (password != NULL && strlen(password) > QMTCONN_PASSWORD_MAX_SIZE)
  {
    ESP_LOGE(TAG, "Password too long (max %d chars)", QMTCONN_PASSWORD_MAX_SIZE);
    return ESP_ERR_INVALID_ARG;
  }

  // Password can only be provided if username is also provided
  if (password != NULL && username == NULL)
  {
    ESP_LOGE(TAG, "Cannot provide password without username");
    return ESP_ERR_INVALID_ARG;
  }

  // Prepare write parameters
  qmtconn_write_params_t params = {.client_idx = client_idx};
  strncpy(params.client_id, client_id_str, sizeof(params.client_id) - 1);
  params.client_id[sizeof(params.client_id) - 1] = '\0'; // Ensure null termination

  if (username != NULL)
  {
    params.present.has_username = true;
    strncpy(params.username, username, sizeof(params.username) - 1);
    params.username[sizeof(params.username) - 1] = '\0'; // Ensure null termination

    if (password != NULL)
    {
      params.present.has_password = true;
      strncpy(params.password, password, sizeof(params.password) - 1);
      params.password[sizeof(params.password) - 1] = '\0'; // Ensure null termination
    }
  }

  ESP_LOGI(TAG, "Connecting MQTT client %d with client ID '%s'", client_idx, client_id_str);

  // Send the command
  qmtconn_write_response_t local_response = {0};
  esp_err_t                err            = at_cmd_handler_send_and_receive_cmd(&handle->at_handler,
                                                      &AT_CMD_QMTCONN,
                                                      AT_CMD_TYPE_WRITE,
                                                      &params,
                                                      response ? response : &local_response);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to send MQTT connect command: %s", esp_err_to_name(err));
    return err;
  }

  // If we got a result code in the immediate response, check if it was successful
  if ((response && response->present.has_result) ||
      (!response && local_response.present.has_result))
  {
    qmtconn_result_t result = response ? response->result : local_response.result;

    if (result != QMTCONN_RESULT_SUCCESS)
    {
      ESP_LOGE(TAG,
               "MQTT connect operation failed with result: %d (%s)",
               result,
               enum_to_str(result, QMTCONN_RESULT_MAP, QMTCONN_RESULT_MAP_SIZE));

      // If we have a return code, log it for more detail
      if ((response && response->present.has_ret_code) ||
          (!response && local_response.present.has_ret_code))
      {
        qmtconn_ret_code_t ret_code = response ? response->ret_code : local_response.ret_code;
        ESP_LOGE(TAG,
                 "MQTT connect return code: %d (%s)",
                 ret_code,
                 enum_to_str(ret_code, QMTCONN_RET_CODE_MAP, QMTCONN_RET_CODE_MAP_SIZE));
      }

      return ESP_FAIL;
    }

    ESP_LOGI(TAG, "MQTT client %d connected successfully", client_idx);
  }
  else
  {
    // If no result code in the immediate response, that's expected
    // The URC with the result will come later, the caller needs to wait for it
    ESP_LOGI(TAG, "MQTT connect command sent successfully, waiting for connection result");
  }

  return ESP_OK;
}

esp_err_t
bg95_mqtt_disconnect(bg95_handle_t* handle, uint8_t client_idx, qmtdisc_write_response_t* response)
{
  if (NULL == handle || !handle->initialized)
  {
    ESP_LOGE(TAG, "Invalid handle or handle not initialized");
    return ESP_ERR_INVALID_ARG;
  }

  if (client_idx > QMTDISC_CLIENT_IDX_MAX)
  {
    ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-%d)", client_idx, QMTDISC_CLIENT_IDX_MAX);
    return ESP_ERR_INVALID_ARG;
  }

  // Prepare write parameters
  qmtdisc_write_params_t params = {.client_idx = client_idx};

  ESP_LOGI(TAG, "Disconnecting MQTT client %d from server", client_idx);

  // Send the command
  qmtdisc_write_response_t local_response = {0};
  esp_err_t                err            = at_cmd_handler_send_and_receive_cmd(&handle->at_handler,
                                                      &AT_CMD_QMTDISC,
                                                      AT_CMD_TYPE_WRITE,
                                                      &params,
                                                      response ? response : &local_response);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to send MQTT disconnect command: %s", esp_err_to_name(err));
    return err;
  }

  // If we got a result code in the immediate response, check if it was successful
  if ((response && response->present.has_result) ||
      (!response && local_response.present.has_result))
  {
    qmtdisc_result_t result = response ? response->result : local_response.result;

    if (result != QMTDISC_RESULT_SUCCESS)
    {
      ESP_LOGE(TAG,
               "MQTT disconnect failed with result: %d (%s)",
               result,
               enum_to_str(result, QMTDISC_RESULT_MAP, QMTDISC_RESULT_MAP_SIZE));
      return ESP_FAIL;
    }

    ESP_LOGI(TAG, "MQTT client %d disconnected successfully", client_idx);
  }
  else
  {
    // If no result code in the immediate response, that's expected
    // The URC with the result will come later, the caller needs to wait for it
    ESP_LOGI(TAG, "MQTT disconnect command sent successfully, waiting for result");
  }

  return ESP_OK;
}

esp_err_t bg95_mqtt_publish_fixed_length(bg95_handle_t*           handle,
                                         uint8_t                  client_idx,
                                         uint16_t                 msgid,
                                         qmtpub_qos_t             qos,
                                         qmtpub_retain_t          retain,
                                         const char*              topic,
                                         const void*              message,
                                         uint16_t                 message_length,
                                         qmtpub_write_response_t* response)
{
  if (NULL == handle || NULL == topic || NULL == message || message_length == 0 ||
      !handle->initialized)
  {
    ESP_LOGE(TAG, "Invalid arguments or handle not initialized");
    return ESP_ERR_INVALID_ARG;
  }

  if (client_idx > QMTPUB_CLIENT_IDX_MAX)
  {
    ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-%d)", client_idx, QMTPUB_CLIENT_IDX_MAX);
    return ESP_ERR_INVALID_ARG;
  }

  if (message_length > QMTPUB_MSG_MAX_LEN)
  {
    ESP_LOGE(TAG, "Message too long: %d bytes (max %d)", message_length, QMTPUB_MSG_MAX_LEN);
    return ESP_ERR_INVALID_ARG;
  }

  // QoS 0 requires msgid to be 0
  if (qos == QMTPUB_QOS_AT_MOST_ONCE && msgid != 0)
  {
    ESP_LOGW(TAG, "QoS 0 requires msgid=0, forcing msgid to 0");
    msgid = 0;
  }

  // For QoS 1 and 2, msgid must be non-zero
  if ((qos == QMTPUB_QOS_AT_LEAST_ONCE || qos == QMTPUB_QOS_EXACTLY_ONCE) && msgid == 0)
  {
    ESP_LOGW(TAG, "QoS %d requires non-zero msgid, using msgid=1", qos);
    msgid = 1;
  }

  // Prepare write parameters
  qmtpub_write_params_t params = {.client_idx = client_idx,
                                  .msgid      = msgid,
                                  .qos        = qos,
                                  .retain     = retain,
                                  .msglen     = message_length};

  strncpy(params.topic, topic, sizeof(params.topic) - 1);
  params.topic[sizeof(params.topic) - 1] = '\0'; // Ensure null termination

  ESP_LOGI(TAG,
           "Publishing fixed-length MQTT message on topic '%s' with QoS %d, client %d, msgid %d, "
           "length %d",
           topic,
           qos,
           client_idx,
           msgid,
           message_length);

  // Send the command with data
  qmtpub_write_response_t local_response = {0};
  esp_err_t               err            = at_cmd_handler_send_with_prompt(&handle->at_handler,
                                                  &AT_CMD_QMTPUB,
                                                  AT_CMD_TYPE_WRITE,
                                                  &params,
                                                  message,
                                                  message_length,
                                                  response ? response : &local_response);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to send MQTT publish command: %s", esp_err_to_name(err));
    return err;
  }

  // If we got a result code in the immediate response, check if it was successful
  if ((response && response->present.has_result) ||
      (!response && local_response.present.has_result))
  {
    qmtpub_result_t result = response ? response->result : local_response.result;

    if (result != QMTPUB_RESULT_SUCCESS)
    {
      ESP_LOGE(TAG,
               "MQTT publish failed with result: %d (%s)",
               result,
               enum_to_str(result, QMTPUB_RESULT_MAP, QMTPUB_RESULT_MAP_SIZE));
      return ESP_FAIL;
    }

    ESP_LOGI(TAG, "MQTT message published successfully");
  }
  else
  {
    // If no result code in the immediate response, that's expected
    // The URC with the result will come later, the caller needs to wait for it
    ESP_LOGI(TAG, "MQTT publish command sent successfully, waiting for publication result");
  }

  return ESP_OK;
}

esp_err_t bg95_mqtt_subscribe(bg95_handle_t*           handle,
                              uint8_t                  client_idx,
                              uint16_t                 msgid,
                              const char*              topic,
                              qmtsub_qos_t             qos,
                              qmtsub_write_response_t* response)
{
  // Validate parameters
  if (NULL == handle || NULL == topic || !handle->initialized)
  {
    ESP_LOGE(TAG, "Invalid arguments or handle not initialized");
    return ESP_ERR_INVALID_ARG;
  }

  if (client_idx > QMTSUB_CLIENT_IDX_MAX)
  {
    ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-%d)", client_idx, QMTSUB_CLIENT_IDX_MAX);
    return ESP_ERR_INVALID_ARG;
  }

  if (msgid < QMTSUB_MSGID_MIN || msgid > QMTSUB_MSGID_MAX)
  {
    ESP_LOGE(TAG, "Invalid msgid: %d (must be %d-%d)", msgid, QMTSUB_MSGID_MIN, QMTSUB_MSGID_MAX);
    return ESP_ERR_INVALID_ARG;
  }

  if (qos > QMTSUB_QOS_EXACTLY_ONCE)
  {
    ESP_LOGE(TAG, "Invalid QoS: %d (must be 0-2)", qos);
    return ESP_ERR_INVALID_ARG;
  }

  if (topic[0] == '\0' || strlen(topic) >= QMTSUB_TOPIC_MAX_SIZE)
  {
    ESP_LOGE(TAG, "Invalid topic: empty or too long (max %d chars)", QMTSUB_TOPIC_MAX_SIZE - 1);
    return ESP_ERR_INVALID_ARG;
  }

  // Prepare write parameters
  qmtsub_write_params_t params = {.client_idx = client_idx, .msgid = msgid, .topic_count = 1};

  // Set the topic and QoS
  strncpy(params.topics[0].topic, topic, QMTSUB_TOPIC_MAX_SIZE - 1);
  params.topics[0].topic[QMTSUB_TOPIC_MAX_SIZE - 1] = '\0'; // Ensure null termination
  params.topics[0].qos                              = qos;

  ESP_LOGI(TAG,
           "Subscribing MQTT client %d to topic '%s' with QoS %d, msgid %d",
           client_idx,
           topic,
           qos,
           msgid);

  // Create local response structure if user didn't provide one
  qmtsub_write_response_t  local_response = {0};
  qmtsub_write_response_t* resp_ptr       = (response != NULL) ? response : &local_response;

  // Send the command
  esp_err_t err = at_cmd_handler_send_and_receive_cmd(
      &handle->at_handler, &AT_CMD_QMTSUB, AT_CMD_TYPE_WRITE, &params, resp_ptr);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to send MQTT subscribe command: %s", esp_err_to_name(err));
    return err;
  }

  // If we got a result code in the immediate response, check if it was successful
  if (resp_ptr->present.has_result)
  {
    if (resp_ptr->result != QMTSUB_RESULT_SUCCESS)
    {
      ESP_LOGE(TAG,
               "MQTT subscribe failed with result: %d (%s)",
               resp_ptr->result,
               enum_to_str(resp_ptr->result, QMTSUB_RESULT_MAP, QMTSUB_RESULT_MAP_SIZE));
      return ESP_FAIL;
    }

    if (resp_ptr->present.has_value)
    {
      ESP_LOGI(TAG, "MQTT subscription successful, granted QoS: %d", resp_ptr->value);
    }
    else
    {
      ESP_LOGI(TAG, "MQTT subscription successful");
    }
  }
  else
  {
    // If no result code in the immediate response, that's expected
    // The URC with the result will come later, the caller needs to wait for it
    ESP_LOGI(TAG, "MQTT subscribe command sent successfully, waiting for subscription result");
  }

  return ESP_OK;
}

esp_err_t bg95_mqtt_unsubscribe(bg95_handle_t*           handle,
                                uint8_t                  client_idx,
                                uint16_t                 msgid,
                                const char*              topic,
                                qmtuns_write_response_t* response)
{
  // Validate parameters
  if (NULL == handle || NULL == topic || !handle->initialized)
  {
    ESP_LOGE(TAG, "Invalid arguments or handle not initialized");
    return ESP_ERR_INVALID_ARG;
  }

  if (client_idx > QMTUNS_CLIENT_IDX_MAX)
  {
    ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-%d)", client_idx, QMTUNS_CLIENT_IDX_MAX);
    return ESP_ERR_INVALID_ARG;
  }

  if (msgid < QMTUNS_MSGID_MIN || msgid > QMTUNS_MSGID_MAX)
  {
    ESP_LOGE(TAG, "Invalid msgid: %d (must be %d-%d)", msgid, QMTUNS_MSGID_MIN, QMTUNS_MSGID_MAX);
    return ESP_ERR_INVALID_ARG;
  }

  if (topic[0] == '\0' || strlen(topic) >= QMTUNS_TOPIC_MAX_SIZE)
  {
    ESP_LOGE(TAG, "Invalid topic: empty or too long (max %d chars)", QMTUNS_TOPIC_MAX_SIZE - 1);
    return ESP_ERR_INVALID_ARG;
  }

  // Prepare write parameters
  qmtuns_write_params_t params = {.client_idx = client_idx, .msgid = msgid, .topic_count = 1};

  // Set the topic
  strncpy(params.topics[0], topic, QMTUNS_TOPIC_MAX_SIZE - 1);
  params.topics[0][QMTUNS_TOPIC_MAX_SIZE - 1] = '\0'; // Ensure null termination

  ESP_LOGI(TAG, "Unsubscribing MQTT client %d from topic '%s', msgid %d", client_idx, topic, msgid);

  // Create local response structure if user didn't provide one
  qmtuns_write_response_t  local_response = {0};
  qmtuns_write_response_t* resp_ptr       = (response != NULL) ? response : &local_response;

  // Send the command
  esp_err_t err = at_cmd_handler_send_and_receive_cmd(
      &handle->at_handler, &AT_CMD_QMTUNS, AT_CMD_TYPE_WRITE, &params, resp_ptr);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to send MQTT unsubscribe command: %s", esp_err_to_name(err));
    return err;
  }

  // If we got a result code in the immediate response, check if it was successful
  if (resp_ptr->present.has_result)
  {
    if (resp_ptr->result != QMTUNS_RESULT_SUCCESS)
    {
      ESP_LOGE(TAG,
               "MQTT unsubscribe failed with result: %d (%s)",
               resp_ptr->result,
               enum_to_str(resp_ptr->result, QMTUNS_RESULT_MAP, QMTUNS_RESULT_MAP_SIZE));
      return ESP_FAIL;
    }

    ESP_LOGI(TAG, "MQTT unsubscribe successful");
  }
  else
  {
    // If no result code in the immediate response, that's expected
    // The URC with the result will come later, the caller needs to wait for it
    ESP_LOGI(TAG, "MQTT unsubscribe command sent successfully, waiting for result");
  }

  return ESP_OK;
}
