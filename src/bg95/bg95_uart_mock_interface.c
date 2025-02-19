#pragma once

#include "bg95_uart_interface.h"
#include "esp_log.h"

#include <string.h>

// NOTE: Was going to make this all static allocated on stack (no dynamic mem usage) - but its only
// for testing so who cares
//  #define AT_CMD_MAX_LEN_UART_INTERFACE 128

// REsponse structure for a single cmd

static const char* TAG = "MOCK UART INTERFACE";

// Helper function to find matching response -- look  up for array of response structs based on
// their expected_cmd
static const mock_uart_response_t* find_matching_response(const mock_uart_state_t* state,
                                                          const char*              command)
{
  for (size_t i = 0; i < state->num_responses; i++)
  {
    if (strstr(command, state->responses[i].expected_cmd) != NULL)
    {
      return &state->responses[i];
    }
  }
  return NULL;
}

static esp_err_t uart_mock_write_impl(const char* data, size_t len, void* context)
{
  if (!data || !context || len == 0)
  {
    return ESP_ERR_INVALID_ARG;
  }

  mock_uart_state_t* state = (mock_uart_state_t*) context;

  // Store the command for later matching
  state->last_received_cmd = data; // Assuming data remains valid

  ESP_LOGI(TAG, "Mock UART write: %.*s", (int) len, data);
  return ESP_OK;
}

static esp_err_t uart_mock_read_impl(
    char* buffer, size_t max_len, size_t* bytes_read, uint32_t timeout_ms, void* context)
{
  if (!buffer || !context || !bytes_read)
  {
    return ESP_ERR_INVALID_ARG;
  }

  mock_uart_state_t* state = (mock_uart_state_t*) context;
  *bytes_read              = 0;

  // Find matching response for last command
  const mock_uart_response_t* response = find_matching_response(state, state->last_received_cmd);
  if (!response)
  {
    ESP_LOGW(TAG, "No matching response found for command: %s", state->last_received_cmd);
    return ESP_ERR_NOT_FOUND;
  }

  // Handle delay if specified
  if (response->delay_ms > 0)
  {
    vTaskDelay(pdMS_TO_TICKS(response->delay_ms));
  }

  // Copy response data
  size_t response_len = strlen(response->cmd_response);
  if (response_len > max_len)
  {
    // IF response is longer than assigned buffer, copy up to the available buffer len of the
    // response
    response_len = max_len;
  }
  memcpy(buffer, response->cmd_response, response_len);
  *bytes_read = response_len;

  ESP_LOGI(TAG, "Mock UART read returned: %.*s", (int) response_len, response->cmd_response);
  return ESP_OK;
}

esp_err_t mock_uart_init(bg95_uart_interface_t*      interface,
                         const mock_uart_response_t* responses,
                         size_t                      num_responses)
{
  if (!interface || !responses || num_responses == 0)
  {
    return ESP_ERR_INVALID_ARG;
  }

  // Allocate state
  mock_uart_state_t* state = calloc(1, sizeof(mock_uart_state_t));
  if (!state)
  {
    return ESP_ERR_NO_MEM;
  }

  // Initialize state
  state->responses         = responses;
  state->num_responses     = num_responses;
  state->last_received_cmd = NULL;

  // Set up interface
  interface->write   = uart_mock_write_impl;
  interface->read    = uart_mock_read_impl;
  interface->context = state;

  return ESP_OK;
}

void mock_uart_deinit(bg95_uart_interface_t* interface)
{
  if (!interface || !interface->context)
  {
    return;
  }

  free(interface->context);
  interface->write   = NULL;
  interface->read    = NULL;
  interface->context = NULL;
}
