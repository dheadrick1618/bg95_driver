#include "at_cmd_qmtconn.h"

#include "at_cmd_structure.h"
#include "esp_err.h"
#include "esp_log.h"

#include <string.h>

static const char* TAG = "AT_CMD_QMTCONN";

// Define the mapping arrays
const enum_str_map_t QMTCONN_RESULT_MAP[QMTCONN_RESULT_MAP_SIZE] = {
    {QMTCONN_RESULT_SUCCESS, "Packet sent successfully and ACK received"},
    {QMTCONN_RESULT_RETRANSMISSION, "Packet retransmission"},
    {QMTCONN_RESULT_FAILED_TO_SEND, "Failed to send a packet"}};

const enum_str_map_t QMTCONN_STATE_MAP[QMTCONN_STATE_MAP_SIZE] = {
    {QMTCONN_STATE_INITIALIZING, "MQTT is initializing"},
    {QMTCONN_STATE_CONNECTING, "MQTT is connecting"},
    {QMTCONN_STATE_CONNECTED, "MQTT is connected"},
    {QMTCONN_STATE_DISCONNECTING, "MQTT is disconnecting"}};

const enum_str_map_t QMTCONN_RET_CODE_MAP[QMTCONN_RET_CODE_MAP_SIZE] = {
    {QMTCONN_RET_CODE_ACCEPTED, "Connection Accepted"},
    {QMTCONN_RET_CODE_UNACCEPTABLE_PROTOCOL, "Connection Refused: Unacceptable Protocol Version"},
    {QMTCONN_RET_CODE_IDENTIFIER_REJECTED, "Connection Refused: Identifier Rejected"},
    {QMTCONN_RET_CODE_SERVER_UNAVAILABLE, "Connection Refused: Server Unavailable"},
    {QMTCONN_RET_CODE_BAD_CREDENTIALS, "Connection Refused: Bad Username or Password"},
    {QMTCONN_RET_CODE_NOT_AUTHORIZED, "Connection Refused: Not Authorized"}};

static esp_err_t qmtconn_test_parser(const char* response, void* parsed_data)
{
  if (NULL == response || NULL == parsed_data)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  qmtconn_test_response_t* test_data = (qmtconn_test_response_t*) parsed_data;

  // Initialize output structure
  memset(test_data, 0, sizeof(qmtconn_test_response_t));

  // Find the response start
  const char* start = strstr(response, "+QMTCONN: ");
  if (!start)
  {
    ESP_LOGE(TAG, "No QMTCONN data in test response");
    return ESP_ERR_INVALID_RESPONSE;
  }

  start += 10; // Skip "+QMTCONN: "

  // Parse the range of supported client_idx values
  // The format is expected to be: "(min-max),<clientID>,<username>,<password>"
  int min_idx, max_idx;
  if (sscanf(start, "(%d-%d)", &min_idx, &max_idx) == 2)
  {
    test_data->client_idx_min = (uint8_t) min_idx;
    test_data->client_idx_max = (uint8_t) max_idx;
    return ESP_OK;
  }

  return ESP_ERR_INVALID_RESPONSE;
}

static esp_err_t qmtconn_read_parser(const char* response, void* parsed_data)
{
  if (NULL == response || NULL == parsed_data)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  qmtconn_read_response_t* read_data = (qmtconn_read_response_t*) parsed_data;

  // Initialize output structure
  memset(read_data, 0, sizeof(qmtconn_read_response_t));

  // Find the response start
  const char* start = strstr(response, "+QMTCONN: ");
  if (!start)
  {
    // No QMTCONN data in response (might just be OK without data)
    // This means no client is currently connected
    return ESP_OK;
  }

  start += 10; // Skip "+QMTCONN: "

  // Parse client_idx and state
  // Format: +QMTCONN: <client_idx>,<state>
  int client_idx, state;
  if (sscanf(start, "%d,%d", &client_idx, &state) == 2)
  {
    if (client_idx >= QMTCONN_CLIENT_IDX_MIN && client_idx <= QMTCONN_CLIENT_IDX_MAX)
    {
      read_data->client_idx             = (uint8_t) client_idx;
      read_data->present.has_client_idx = true;
    }
    else
    {
      ESP_LOGW(TAG, "Invalid client_idx in response: %d", client_idx);
    }

    if (state >= QMTCONN_STATE_INITIALIZING && state <= QMTCONN_STATE_DISCONNECTING)
    {
      read_data->state             = (qmtconn_state_t) state;
      read_data->present.has_state = true;
    }
    else
    {
      ESP_LOGW(TAG, "Invalid state in response: %d", state);
    }

    return ESP_OK;
  }

  return ESP_ERR_INVALID_RESPONSE;
}

static esp_err_t qmtconn_write_formatter(const void* params, char* buffer, size_t buffer_size)
{
  if (NULL == params || NULL == buffer || 0 == buffer_size)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  const qmtconn_write_params_t* write_params = (const qmtconn_write_params_t*) params;

  // Validate parameters
  if (write_params->client_idx > QMTCONN_CLIENT_IDX_MAX)
  {
    ESP_LOGE(TAG,
             "Invalid client_idx: %d (must be 0-%d)",
             write_params->client_idx,
             QMTCONN_CLIENT_IDX_MAX);
    return ESP_ERR_INVALID_ARG;
  }

  if (write_params->client_id[0] == '\0')
  {
    ESP_LOGE(TAG, "Client ID cannot be empty");
    return ESP_ERR_INVALID_ARG;
  }

  // If password is provided, username must also be provided
  if (write_params->present.has_password && !write_params->present.has_username)
  {
    ESP_LOGE(TAG, "Username must be provided when password is provided");
    return ESP_ERR_INVALID_ARG;
  }

  int written = 0;

  // Format with required parameters: client_idx and client_id
  written = snprintf(
      buffer, buffer_size, "=%d,\"%s\"", write_params->client_idx, write_params->client_id);

  // Check for buffer overflow
  if (written < 0 || (size_t) written >= buffer_size)
  {
    ESP_LOGE(TAG, "Buffer too small for QMTCONN write command");
    if (buffer_size > 0)
    {
      buffer[0] = '\0'; // Ensure null-terminated in case of overflow
    }
    return ESP_ERR_INVALID_SIZE;
  }

  // Add optional username if provided
  if (write_params->present.has_username)
  {
    int username_written =
        snprintf(buffer + written, buffer_size - written, ",\"%s\"", write_params->username);

    if (username_written < 0 || (size_t) (written + username_written) >= buffer_size)
    {
      ESP_LOGE(TAG, "Buffer too small to include username");
      return ESP_ERR_INVALID_SIZE;
    }

    written += username_written;

    // Add optional password if provided
    if (write_params->present.has_password)
    {
      int password_written =
          snprintf(buffer + written, buffer_size - written, ",\"%s\"", write_params->password);

      if (password_written < 0 || (size_t) (written + password_written) >= buffer_size)
      {
        ESP_LOGE(TAG, "Buffer too small to include password");
        return ESP_ERR_INVALID_SIZE;
      }

      written += password_written;
    }
  }

  return ESP_OK;
}

static esp_err_t qmtconn_write_parser(const char* response, void* parsed_data)
{
  if (NULL == response || NULL == parsed_data)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  qmtconn_write_response_t* write_resp = (qmtconn_write_response_t*) parsed_data;

  // Initialize output structure
  memset(write_resp, 0, sizeof(qmtconn_write_response_t));

  // URC response format: +QMTCONN: <client_idx>,<result>[,<ret_code>]
  const char* result_start = strstr(response, "+QMTCONN: ");
  if (!result_start)
  {
    // The response might not contain the URC yet (just the OK)
    // This is normal because the URC might come later
    return ESP_OK;
  }

  result_start += 10; // Skip "+QMTCONN: "

  int client_idx, result, ret_code;
  int matched_fields = sscanf(result_start, "%d,%d,%d", &client_idx, &result, &ret_code);

  if (matched_fields >= 2) // At least client_idx and result are required
  {
    if (client_idx >= QMTCONN_CLIENT_IDX_MIN && client_idx <= QMTCONN_CLIENT_IDX_MAX)
    {
      write_resp->client_idx             = (uint8_t) client_idx;
      write_resp->present.has_client_idx = true;
    }
    else
    {
      ESP_LOGW(TAG, "Invalid client_idx in response: %d", client_idx);
    }

    // Set the result code
    if (result >= QMTCONN_RESULT_SUCCESS && result <= QMTCONN_RESULT_FAILED_TO_SEND)
    {
      write_resp->result             = (qmtconn_result_t) result;
      write_resp->present.has_result = true;
    }
    else
    {
      ESP_LOGW(TAG, "Invalid result in response: %d", result);
    }

    // If ret_code is present (optional)
    if (matched_fields >= 3)
    {
      if (ret_code >= QMTCONN_RET_CODE_ACCEPTED && ret_code <= QMTCONN_RET_CODE_NOT_AUTHORIZED)
      {
        write_resp->ret_code             = (qmtconn_ret_code_t) ret_code;
        write_resp->present.has_ret_code = true;
      }
      else
      {
        ESP_LOGW(TAG, "Invalid return code in response: %d", ret_code);
      }
    }

    // Log result for debugging
    ESP_LOGI(TAG,
             "QMTCONN operation result: %d (%s)",
             result,
             enum_to_str(result, QMTCONN_RESULT_MAP, QMTCONN_RESULT_MAP_SIZE));

    if (write_resp->present.has_ret_code)
    {
      ESP_LOGI(TAG,
               "QMTCONN return code: %d (%s)",
               ret_code,
               enum_to_str(ret_code, QMTCONN_RET_CODE_MAP, QMTCONN_RET_CODE_MAP_SIZE));
    }

    return ESP_OK;
  }

  return ESP_ERR_INVALID_RESPONSE;
}

// Command definition for QMTCONN
const at_cmd_t AT_CMD_QMTCONN = {
    .name        = "QMTCONN",
    .description = "Connect a Client to MQTT Server",
    .type_info   = {[AT_CMD_TYPE_TEST]    = {.parser        = qmtconn_test_parser,
                                             .formatter     = NULL,
                                             .response_type = AT_CMD_RESPONSE_TYPE_DATA_REQUIRED},
                    [AT_CMD_TYPE_READ]    = {.parser        = qmtconn_read_parser,
                                             .formatter     = NULL,
                                             .response_type = AT_CMD_RESPONSE_TYPE_DATA_OPTIONAL},
                    [AT_CMD_TYPE_WRITE]   = {.parser        = qmtconn_write_parser,
                                             .formatter     = qmtconn_write_formatter,
                                             .response_type = AT_CMD_RESPONSE_TYPE_DATA_REQUIRED},
                    [AT_CMD_TYPE_EXECUTE] = AT_CMD_TYPE_DOES_NOT_EXIST},
    .timeout_ms  = 5000 // Default packet timeout is 5 seconds per specification
};
