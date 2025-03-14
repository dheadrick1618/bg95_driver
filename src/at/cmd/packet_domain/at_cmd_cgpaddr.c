
#include "at_cmd_cgpaddr.h"

#include "at_cmd_structure.h"
#include "esp_log.h"

#include <string.h>

static const char* TAG = "AT_CMD_CGPADDR";

static esp_err_t cgpaddr_write_response_parser(const char* response, void* parsed_data)
{
  if (NULL == response || NULL == parsed_data)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  cgpaddr_write_response_t* read_data = (cgpaddr_write_response_t*) parsed_data;

  /* Initialize output structure */
  (void) memset(read_data, 0, sizeof(cgpaddr_write_response_t));

  const char* start = strstr(response, "+CGPADDR: ");
  if (!start)
  {
    return ESP_ERR_INVALID_RESPONSE;
  }
  start += 10; // Skip  "+CGPADDR: "

  /* Parse CID value */
  int cid = 0;
  if (sscanf(start, "%d,", &cid) != 1)
  {
    return ESP_ERR_INVALID_RESPONSE;
  }

  read_data->cid = cid;

  /* Find the address part after the comma */
  const char* addr_start = strchr(start, ',');
  if (!addr_start)
  {
    return ESP_ERR_INVALID_RESPONSE;
  }
  addr_start++; // Skip the comma

  /* Find the end of the line */
  const char* addr_end = strstr(addr_start, "\r\n");
  if (!addr_end)
  {
    addr_end = addr_start + strlen(addr_start); // If no CRLF, use end of string
  }

  /* Calculate address length and copy it */
  size_t addr_len = addr_end - addr_start;
  if (addr_len >= CGPADDR_ADDRESS_MAX_CHARS)
  {
    addr_len = CGPADDR_ADDRESS_MAX_CHARS - 1; // Truncate if too long
  }

  strncpy(read_data->address, addr_start, addr_len);
  read_data->address[addr_len] = '\0'; // Ensure null termination

  return ESP_OK;
}

static esp_err_t cgpaddr_write_formatter(const void* params, char* buffer, size_t buffer_size)
{
  if (!params || !buffer || buffer_size == 0)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  const cgpaddr_write_params_t* write_params = (const cgpaddr_write_params_t*) params;
  int                           written      = 0;

  // Validate CID range
  if (write_params->cid < CGPADDR_CID_RANGE_MIN_VALUE ||
      write_params->cid > CGPADDR_CID_RANGE_MAX_VALUE)
  {
    ESP_LOGE(TAG, "Invalid CID: %d (must be 1-15)", write_params->cid);
    return ESP_ERR_INVALID_ARG;
  }

  written = snprintf(buffer, buffer_size, "=%d", write_params->cid);

  if ((written < 0) || ((size_t) written >= buffer_size))
  {
    /* Ensure null termination in case of overflow */
    if (buffer_size > 0U)
    {
      buffer[0] = '\0';
    }
    return ESP_ERR_INVALID_SIZE;
  }

  return ESP_OK;
}

const at_cmd_t AT_CMD_CGPADDR = {
    .name        = "CGPADDR",
    .description = "Define PDP Context",
    .type_info   = {[AT_CMD_TYPE_TEST]    = AT_CMD_TYPE_NOT_IMPLEMENTED,
                    [AT_CMD_TYPE_READ]    = AT_CMD_TYPE_DOES_NOT_EXIST,
                    [AT_CMD_TYPE_WRITE]   = {.parser    = cgpaddr_write_response_parser,
                                             .formatter = cgpaddr_write_formatter},
                    [AT_CMD_TYPE_EXECUTE] = AT_CMD_TYPE_DOES_NOT_EXIST},
    .timeout_ms  = 300 // 300ms per spec
};
