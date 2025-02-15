#pragma once
#include <driver/uart.h>
#include <esp_err.h>
#include <stddef.h>

#define BG95_BAUD_RATE 115200
#define BG95_UART_BUFF_SIZE 2048

typedef struct
{
  int tx_gpio_num;
  int rx_gpio_num;
  int port_num;
} bg95_uart_config_t;

typedef struct
{
  char*    expected_cmd; // Expected command string
  char*    cmd_response; // Response to that command
  uint32_t delay_ms;     // Delay before emitting response
} mock_uart_response_t;

// State manager for entire mock UART interface
typedef struct
{
  const mock_uart_response_t*
         responses; // const ptr - because responses doesnt change during lifetime of state struct
  size_t num_responses;
  const char* last_received_cmd; // const ptr - because  command data wont change
} mock_uart_state_t;

typedef esp_err_t (*uart_write_fn)(const char* data, size_t len, void* context);
typedef esp_err_t (*uart_read_fn)(
    char* buffer, size_t max_len, size_t* bytes_read, uint32_t timeout_ms, void* context);

typedef struct
{
  uart_write_fn write;
  uart_read_fn  read;
  void*         context;
  uart_port_t   uart_num;
} bg95_uart_interface_t;

// Real UART implementation
esp_err_t bg95_uart_interface_init_hw(bg95_uart_interface_t* interface, bg95_uart_config_t config);

// TODO:
//  esp_err_t bg95_uart_interface_deinit_hw()

// Mock UART implementation for testing
// esp_err_t bg95_uart_interface_init_mock(bg95_uart_interface_t* interface, const char**
// responses);

esp_err_t mock_uart_init(bg95_uart_interface_t*      interface,
                         const mock_uart_response_t* responses,
                         size_t                      num_responses);

void mock_uart_deinit(bg95_uart_interface_t* interface);

// For testing hardware connection
esp_err_t bg95_uart_interface_loopback_test(bg95_uart_interface_t* interface);
esp_err_t bg95_uart_interface_test_at(bg95_uart_interface_t* interface);
