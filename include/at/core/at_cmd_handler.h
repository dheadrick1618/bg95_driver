#pragma once

#include "at_cmd_structure.h"
#include "bg95_uart_interface.h"

#define AT_CRLF "\r\n"
#define AT_OK AT_CRLF "OK" AT_CRLF
#define AT_ERROR AT_CRLF "ERROR" AT_CRLF
#define AT_CME_ERROR AT_CRLF "+CME ERROR:"

#define AT_CMD_READ_CHUNK_SIZE 32
#define AT_CMD_READ_CHUNK_INTERVAL_MS 100

// Uses a provided UART interface - then either real HW or a mock TEST UART can be used interchangably
// -  no  mutex because this assumed to be used as a singleton
typedef struct {
    bg95_uart_interface_t uart;
} at_cmd_handler_t;

// Initialize AT command handler
esp_err_t at_cmd_handler_init(at_cmd_handler_t* handler, bg95_uart_interface_t* uart);

// Send AT command and get response
esp_err_t at_cmd_handler_send_and_receive_cmd(
    at_cmd_handler_t* handler,
    const at_cmd_t* cmd,
    at_cmd_type_t type,
    const void* params,
    at_cmd_response_t* response
);

