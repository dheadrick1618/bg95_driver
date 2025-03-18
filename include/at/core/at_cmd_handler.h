#pragma once

#include "at_cmd_parser.h"
#include "at_cmd_structure.h"
#include "bg95_uart_interface.h"

#include <esp_err.h>
#include <stdbool.h>

#define AT_CRLF "\r\n"
#define AT_OK AT_CRLF "OK" AT_CRLF
#define AT_ERROR AT_CRLF "ERROR" AT_CRLF
#define AT_CME_ERROR AT_CRLF "+CME ERROR:"

#define AT_CMD_READ_CHUNK_SIZE 32
#define AT_CMD_READ_CHUNK_INTERVAL_MS 100
#define AT_CMD_MAX_RESPONSE_LEN 2048
#define AT_CMD_MAX_CMD_LEN 256

// Uses a provided UART interface - then either real HW or a mock TEST UART can be used
// -  no  mutex because this assumed to be used as a singleton
typedef struct
{
  bg95_uart_interface_t uart;
} at_cmd_handler_t;

// Initialize AT command handler - it can be init either with mock or hardware(real) UART interface
esp_err_t at_cmd_handler_init(at_cmd_handler_t* handler, bg95_uart_interface_t* uart);

// Only reason this is not static is for ease of testing
bool has_command_terminated(const char* raw_response, const at_cmd_t* cmd, at_cmd_type_t type);

esp_err_t validate_basic_response(const char* raw_response, at_parsed_response_t* parsed_base);

esp_err_t parse_at_cmd_specific_data_response(const at_cmd_t*             cmd,
                                              at_cmd_type_t               type,
                                              const char*                 raw_response,
                                              const at_parsed_response_t* parsed_base,
                                              void*                       response_data);

// This is only not static for ease of testing
// This contains a loop which reads the UART buffer in chunks, either until a complete response is
// read OR timeout occurs
esp_err_t read_at_cmd_response(at_cmd_handler_t* handler,
                               const at_cmd_t*   cmd,
                               at_cmd_type_t     type,
                               char*             response_buffer,
                               size_t            buffer_size);

// Send AT command and get response
esp_err_t at_cmd_handler_send_and_receive_cmd(
    at_cmd_handler_t* handler,
    const at_cmd_t*   cmd,
    at_cmd_type_t     type,
    const void*       params, // Params for write commands
    void*             response_data);     // Response structure - specific to command and type

// Add to at_cmd_handler.h
esp_err_t at_cmd_handler_send_with_prompt(at_cmd_handler_t* handler,
                                          const at_cmd_t*   cmd,
                                          at_cmd_type_t     type,
                                          const void*       params,
                                          const void*       data,
                                          size_t            data_len,
                                          void*             response_data);
