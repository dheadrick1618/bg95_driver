
#include "bg95_uart_interface.h"

#include "at_cmd_handler.h"
#include "driver/uart.h"

#include <esp_err.h>
#include <esp_log.h>
#include <string.h> //for memset

static const char* TAG = "BG95_UART_INTERFACE";

// Add these function implementations
static esp_err_t uart_hw_write_impl(const char* data, size_t len, void* context)
{
  bg95_uart_interface_t* interface = (bg95_uart_interface_t*) context;
  if (!interface || !data || len == 0)
  {
    return ESP_ERR_INVALID_ARG;
  }

  // Get how much space is available in TX buffer
  size_t    available = 0;
  esp_err_t err       = uart_get_tx_buffer_free_size(interface->uart_num, &available);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to get TX buffer size: %s", esp_err_to_name(err));
    return err;
  }

  if (available < len)
  {
    ESP_LOGW(TAG,
             "TX buffer doesn't have enough space. Available: %d, Need: %d. Flushing...",
             (int) available,
             (int) len);
    // Flush both buffers
    uart_flush(interface->uart_num);

    // Check space again
    err = uart_get_tx_buffer_free_size(interface->uart_num, &available);
    if (err != ESP_OK || available < len)
    {
      ESP_LOGE(TAG, "Still not enough space after flush");
      return ESP_ERR_NO_MEM;
    }
  }

  // Write data in chunks if needed
  size_t written = 0;
  while (written < len)
  {
    size_t chunk_size = len - written;
    int    result     = uart_write_bytes(interface->uart_num, data + written, chunk_size);
    if (result < 0)
    {
      ESP_LOGE(TAG, "Failed to write to UART");
      return ESP_FAIL;
    }
    written += result;
  }

  // Wait for transmission to complete
  err = uart_wait_tx_done(interface->uart_num, pdMS_TO_TICKS(1000));
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to wait for TX complete: %s", esp_err_to_name(err));
    return err;
  }

  return ESP_OK;
}

static esp_err_t uart_hw_read_impl(
    char* buffer, size_t max_len, size_t* bytes_read, uint32_t timeout_ms, void* context)
{
  bg95_uart_interface_t* interface = (bg95_uart_interface_t*) context;
  if (!interface || !buffer || !bytes_read)
  {
    return ESP_ERR_INVALID_ARG;
  }

  TickType_t ticks = pdMS_TO_TICKS(timeout_ms);
  *bytes_read      = 0;

  int len = uart_read_bytes(interface->uart_num, (uint8_t*) buffer, max_len - 1, ticks);
  if (len < 0)
  {
    return ESP_FAIL;
  }

  // -------------------------------------------------------
  // NOTE: FOR DEBUG
  // ESP_LOGI(TAG, "UART read: %d bytes", len);
  // if (len > 0)
  // {
  //   buffer[len] = '\0';
  //   ESP_LOGI(TAG, "UART data: %s", buffer);
  //
  //   // Log hex dump for debugging non-printable characters
  //   ESP_LOG_BUFFER_HEXDUMP(TAG, buffer, len, ESP_LOG_INFO);
  // }
  //---------------------------------------------------------

  *bytes_read = len;
  buffer[len] = '\0';
  return ESP_OK;
}

// Real UART implementation
esp_err_t bg95_uart_interface_init_hw(bg95_uart_interface_t* interface, bg95_uart_config_t config)
{

  if (!interface)
  {
    ESP_LOGE(TAG, "Interface is NULL");
    return ESP_ERR_INVALID_ARG;
  }

  ESP_LOGI(TAG,
           "Initializing UART interface with params: \nport %d\ntx pin: %d\nrx pin: %d",
           config.port_num,
           config.tx_gpio_num,
           config.rx_gpio_num);

  // Clear interface first
  memset(interface, 0, sizeof(bg95_uart_interface_t));

  interface->uart_num = (uart_port_t) config.port_num;
  interface->context  = interface; // Store self as context
  interface->write    = uart_hw_write_impl;
  interface->read     = uart_hw_read_impl;

  uart_config_t uart_config = {
      .baud_rate  = BG95_BAUD_RATE,
      .data_bits  = UART_DATA_8_BITS,
      .parity     = UART_PARITY_DISABLE,
      .stop_bits  = UART_STOP_BITS_1,
      .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_DEFAULT,
  };

  ESP_LOGI(TAG, "Installing UART driver");

  esp_err_t err = uart_driver_install(
      (uart_port_t) config.port_num, BG95_UART_BUFF_SIZE * 2, BG95_UART_BUFF_SIZE * 2, 0, NULL, 0);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Error installing UART driver: %s", esp_err_to_name(err));
    return err;
  }

  // After installation, let's verify our buffers:
  size_t tx_size = 0;
  err            = uart_get_tx_buffer_free_size(interface->uart_num, &tx_size);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to get TX buffer size");
    return err;
  }
  ESP_LOGI(TAG, "TX buffer available space: %d bytes", (int) tx_size);

  err = uart_param_config((uart_port_t) config.port_num, &uart_config);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Error configuring UART parameters: %s", esp_err_to_name(err));
    uart_driver_delete(interface->uart_num);
    return err;
  }

  err = uart_set_pin((uart_port_t) config.port_num,
                     config.tx_gpio_num,
                     config.rx_gpio_num,
                     UART_PIN_NO_CHANGE,
                     UART_PIN_NO_CHANGE);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Error setting UART pins: %s", esp_err_to_name(err));
    uart_driver_delete(interface->uart_num);
    return err;
  }

  uart_flush(interface->uart_num);

  vTaskDelay(pdMS_TO_TICKS(100));

  ESP_LOGI(TAG, "UART driver installed");
  return ESP_OK;
}

// Mock UART implementation for testing
esp_err_t bg95_uart_interface_init_mock(bg95_uart_interface_t* interface, const char** responses)
{
  // TODO
  return ESP_FAIL;
}

esp_err_t bg95_uart_interface_loopback_test(bg95_uart_interface_t* interface)
{
  if (!interface)
  {
    ESP_LOGE(TAG, "Interface is NULL");
    return ESP_ERR_INVALID_ARG;
  }

  const char test_str[]    = "UART Loopback Test";
  char       rx_buffer[32] = {0};
  size_t     bytes_read    = 0;

  ESP_LOGI(TAG, "Starting UART loopback test...");

  // First, flush any existing data
  uart_flush(interface->uart_num);

  // Send test string
  esp_err_t err = interface->write(test_str, strlen(test_str), interface->context);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to write test string");
    return err;
  }

  // Add small delay to ensure transmission is complete
  vTaskDelay(pdMS_TO_TICKS(10));

  // Try to read back what we sent
  err = interface->read(rx_buffer, sizeof(rx_buffer) - 1, &bytes_read, 1000, interface->context);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to read response");
    return err;
  }

  ESP_LOGI(TAG, "Sent: %s", test_str);
  ESP_LOGI(TAG, "Received: %s", rx_buffer);
  ESP_LOGI(TAG, "Bytes read: %d", (int) bytes_read);

  // Compare what we sent with what we received
  if (bytes_read != strlen(test_str) || memcmp(test_str, rx_buffer, strlen(test_str)) != 0)
  {
    ESP_LOGE(TAG, "Loopback test failed - data mismatch");
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Loopback test passed!");
  return ESP_OK;
}

esp_err_t bg95_uart_interface_test_at(bg95_uart_interface_t* interface)
{
  if (!interface)
  {
    ESP_LOGE(TAG, "Interface is NULL");
    return ESP_ERR_INVALID_ARG;
  }

  const char test_cmd[]     = "AT\r\n";
  char       rx_buffer[128] = {0};
  size_t     bytes_read     = 0;

  ESP_LOGI(TAG, "Sending AT test command...");

  // First flush any existing data
  uart_flush(interface->uart_num);

  // Send AT command
  esp_err_t err = interface->write(test_cmd, strlen(test_cmd), interface->context);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to write AT command");
    return err;
  }

  // Read response with timeout
  err = interface->read(rx_buffer, sizeof(rx_buffer) - 1, &bytes_read, 1000, interface->context);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to read response");
    return err;
  }

  ESP_LOGI(TAG, "Sent: %s", test_cmd);
  ESP_LOGI(TAG, "Received (%d bytes): %s", bytes_read, rx_buffer);

  // Check for OK or ERROR in response
  if (strstr(rx_buffer, "OK") != NULL)
  {
    ESP_LOGI(TAG, "AT test successful - received OK");
    return ESP_OK;
  }
  else if (strstr(rx_buffer, "ERROR") != NULL)
  {
    ESP_LOGE(TAG, "AT test failed - received ERROR");
    return ESP_FAIL;
  }
  else
  {
    ESP_LOGE(TAG, "AT test failed - unexpected response");
    return ESP_FAIL;
  }
}
