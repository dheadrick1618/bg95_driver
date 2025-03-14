#include "at_cmd_cgdcont.h"

#include "at_cmd_structure.h"
#include "enum_utils.h"

#include <esp_log.h>
#include <string.h> // for strstr, strchr, memset, etc.

static const char* TAG = "AT_CMD_CGDCONT";

// Define the mapping arrays
const enum_str_map_t CGDCONT_PDP_TYPE_MAP[CGDCONT_PDP_TYPE_MAP_SIZE] = {
    {CGDCONT_PDP_TYPE_IP, "IP"},
    {CGDCONT_PDP_TYPE_PPP, "PPP"},
    {CGDCONT_PDP_TYPE_IPV6, "IPV6"},
    {CGDCONT_PDP_TYPE_IPV4V6, "IPV4V6"},
    {CGDCONT_PDP_TYPE_NON_IP, "Non-IP"}};

const enum_str_map_t CGDCONT_DATA_COMP_MAP[CGDCONT_DATA_COMP_MAP_SIZE] = {
    {CGDCONT_DATA_COMP_OFF, "OFF"},
    {CGDCONT_DATA_COMP_ON, "ON"},
    {CGDCONT_DATA_COMP_V42BIS, "V.42bis"}};

const enum_str_map_t CGDCONT_HEAD_COMP_MAP[CGDCONT_HEAD_COMP_MAP_SIZE] = {
    {CGDCONT_HEAD_COMP_OFF, "OFF"},
    {CGDCONT_HEAD_COMP_ON, "ON"},
    {CGDCONT_HEAD_COMP_RFC1144, "RFC 1144"},
    {CGDCONT_HEAD_COMP_RFC2507, "RFC 2507"},
    {CGDCONT_HEAD_COMP_RFC3095, "RFC 3095"}};

const enum_str_map_t CGDCONT_IPV4_ADDR_ALLOC_MAP[CGDCONT_IPV4_ADDR_ALLOC_MAP_SIZE] = {
    {CGDCONT_IPV4_ADDR_ALLOC_NAS, "NAS signaling"}};

// Parser for test command
// static esp_err_t cgdcont_test_parser(const char* response, void* parsed_data)
// {
//   if (!response || !parsed_data)
//   {
//     ESP_LOGE(TAG, "Invalid arguments");
//     return ESP_ERR_INVALID_ARG;
//   }
//
//   cgdcont_test_response_t* test_data = (cgdcont_test_response_t*) parsed_data;
//   memset(test_data, 0, sizeof(cgdcont_test_response_t));
//
//   // Find response start
//   const char* start = strstr(response, "+CGDCONT: ");
//   if (!start)
//   {
//     ESP_LOGE(TAG, "Failed to find +CGDCONT: in response");
//     return ESP_ERR_INVALID_RESPONSE;
//   }
//   start += 10; // Skip "+CGDCONT: "
//
//   // Parse CID range
//   if (sscanf(start, "(%hhu-%hhu)", &test_data->min_cid, &test_data->max_cid) != 2)
//   {
//     ESP_LOGE(TAG, "Failed to parse CID range");
//     return ESP_ERR_INVALID_RESPONSE;
//   }
//
//   // Look for PDP types
//   if (strstr(response, "\"IP\""))
//     test_data->supports_pdp_type_ip = true;
//   if (strstr(response, "\"PPP\""))
//     test_data->supports_pdp_type_ppp = true;
//   if (strstr(response, "\"IPV6\""))
//     test_data->supports_pdp_type_ipv6 = true;
//   if (strstr(response, "\"IPV4V6\""))
//     test_data->supports_pdp_type_ipv4v6 = true;
//   if (strstr(response, "\"Non-IP\""))
//     test_data->supports_pdp_type_non_ip = true;
//
//   // Parse data compression support
//   const char* data_comp = strstr(response, "(list of supported <data_comp>");
//   if (data_comp)
//   {
//     if (strstr(data_comp, "0"))
//       test_data->supports_data_comp[CGDCONT_DATA_COMP_OFF] = true;
//     if (strstr(data_comp, "1"))
//       test_data->supports_data_comp[CGDCONT_DATA_COMP_ON] = true;
//     if (strstr(data_comp, "2"))
//       test_data->supports_data_comp[CGDCONT_DATA_COMP_V42BIS] = true;
//   }
//
//   // Parse header compression support
//   const char* head_comp = strstr(response, "(list of supported <head_comp>");
//   if (head_comp)
//   {
//     if (strstr(head_comp, "0"))
//       test_data->supports_head_comp[CGDCONT_HEAD_COMP_OFF] = true;
//     if (strstr(head_comp, "1"))
//       test_data->supports_head_comp[CGDCONT_HEAD_COMP_ON] = true;
//     if (strstr(head_comp, "2"))
//       test_data->supports_head_comp[CGDCONT_HEAD_COMP_RFC1144] = true;
//     if (strstr(head_comp, "3"))
//       test_data->supports_head_comp[CGDCONT_HEAD_COMP_RFC2507] = true;
//     if (strstr(head_comp, "4"))
//       test_data->supports_head_comp[CGDCONT_HEAD_COMP_RFC3095] = true;
//   }
//
//   // Parse IPv4 address allocation method support
//   const char* ipv4_alloc = strstr(response, "(list of supported <IPv4AddrAlloc>");
//   if (ipv4_alloc)
//   {
//     if (strstr(ipv4_alloc, "0"))
//       test_data->supports_ipv4_addr_alloc[CGDCONT_IPV4_ADDR_ALLOC_NAS] = true;
//   }
//
//   return ESP_OK;
// }

static esp_err_t cgdcont_read_parser(const char* response, void* parsed_data)
{
  /* Validate input parameters */
  if (NULL == response || NULL == parsed_data)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  cgdcont_read_response_t* read_data = (cgdcont_read_response_t*) parsed_data;

  /* Initialize output structure */
  (void) memset(read_data, 0, sizeof(cgdcont_read_response_t));

  const char* line_start = response;
  const char* next_line  = NULL;

  /* Process each line in response */
  while ((line_start = strstr(line_start, "+CGDCONT: ")) != NULL &&
         read_data->num_contexts < 15) /* Bounds check for maximum contexts */
  {
    line_start += 10; /* Skip "+CGDCONT: " */

    /* Find the end of this line */
    next_line = strstr(line_start, "\r\n");
    if (NULL == next_line)
    {
      break; /* No more complete lines */
    }

    /* Check for empty fields (malformed response) */
    if (next_line == line_start)
    {
      /* Empty field, skip to next line */
      line_start = next_line + 2;
      continue;
    }

    /* Ensure we have enough space for another context */
    if (read_data->num_contexts >= 15)
    {
      break; /* Maximum contexts reached */
    }

    cgdcont_pdp_context_t* ctx = &read_data->contexts[read_data->num_contexts];

    /* Initialize context structure */
    (void) memset(ctx, 0, sizeof(cgdcont_pdp_context_t));

    /* Parse CID (required field) */
    int cid = 0;
    if (sscanf(line_start, "%d", &cid) != 1)
    {
      line_start = next_line + 2; /* Skip to next line */
      continue;
    }

    /* Validate CID range for uint8_t */
    if (cid < 0 || cid > 15)
    {
      line_start = next_line + 2;
      continue;
    }

    ctx->cid             = (uint8_t) cid;
    ctx->present.has_cid = true;

    /* Move past the CID to the next field */
    const char* pos = line_start;
    while (*pos != ',' && pos < next_line)
    {
      pos++;
    }

    /* Check if we reached a comma or end of line */
    if (*pos != ',' || pos >= next_line)
    {
      read_data->num_contexts++;
      line_start = next_line + 2;
      continue;
    }
    pos++; /* Skip comma */

    /* Parse PDP type */
    if (*pos == '"' && (pos + 1) < next_line)
    {
      pos++; /* Skip opening quote */
      const char* pdp_type_end = strchr(pos, '"');

      /* Validate pdp_type_end is within this line */
      if (pdp_type_end != NULL && pdp_type_end < next_line)
      {
        size_t pdp_type_len = pdp_type_end - pos;

        /* Ensure we have space in our buffer with room for null terminator */
        if (pdp_type_len < 31) /* 32-1 to ensure space for null terminator */
        {
          char pdp_type_str[32] = {0};
          (void) strncpy(pdp_type_str, pos, pdp_type_len);
          pdp_type_str[pdp_type_len] = '\0'; /* Ensure null-terminated */

          /* Map string to enum */
          for (int i = 0; i < CGDCONT_PDP_TYPE_MAP_SIZE; i++)
          {
            if (strcmp(pdp_type_str, CGDCONT_PDP_TYPE_MAP[i].string) == 0)
            {
              ctx->pdp_type             = (cgdcont_pdp_type_t) CGDCONT_PDP_TYPE_MAP[i].value;
              ctx->present.has_pdp_type = true;
              break;
            }
          }

          pos = pdp_type_end + 1; /* Move past closing quote */
        }
      }
    }

    /* Skip to next field with bounds checking */
    while (*pos != ',' && pos < next_line)
    {
      pos++;
    }

    /* Check if we reached a comma or end of line */
    if (*pos != ',' || pos >= next_line)
    {
      read_data->num_contexts++;
      line_start = next_line + 2;
      continue;
    }
    pos++; /* Skip comma */

    /* Parse APN */
    if (*pos == '"' && (pos + 1) < next_line)
    {
      pos++; /* Skip opening quote */
      const char* apn_end = strchr(pos, '"');

      /* Validate apn_end is within this line */
      if (apn_end != NULL && apn_end < next_line)
      {
        size_t apn_len     = apn_end - pos;
        size_t max_apn_len = sizeof(ctx->apn) - 1; /* Leave room for null terminator */

        if (apn_len <= max_apn_len)
        {
          (void) strncpy(ctx->apn, pos, apn_len);
          ctx->apn[apn_len]    = '\0'; /* Ensure null-terminated */
          ctx->present.has_apn = true;
          pos                  = apn_end + 1; /* Move past closing quote */
        }
      }
    }

    /* Skip to next field with bounds checking */
    while (*pos != ',' && pos < next_line)
    {
      pos++;
    }

    /* Check if we reached a comma or end of line */
    if (*pos != ',' || pos >= next_line)
    {
      read_data->num_contexts++;
      line_start = next_line + 2;
      continue;
    }
    pos++; /* Skip comma */

    /* Parse PDP address */
    if (*pos == '"' && (pos + 1) < next_line)
    {
      pos++; /* Skip opening quote */
      const char* addr_end = strchr(pos, '"');

      /* Validate addr_end is within this line */
      if (addr_end != NULL && addr_end < next_line)
      {
        size_t addr_len     = addr_end - pos;
        size_t max_addr_len = sizeof(ctx->pdp_addr) - 1; /* Leave room for null terminator */

        if (addr_len <= max_addr_len)
        {
          (void) strncpy(ctx->pdp_addr, pos, addr_len);
          ctx->pdp_addr[addr_len]   = '\0'; /* Ensure null-terminated */
          ctx->present.has_pdp_addr = true;
          pos                       = addr_end + 1; /* Move past closing quote */
        }
      }
    }

    /* Skip to next field with bounds checking */
    while (*pos != ',' && pos < next_line)
    {
      pos++;
    }

    /* Check if we reached a comma or end of line */
    if (*pos != ',' || pos >= next_line)
    {
      read_data->num_contexts++;
      line_start = next_line + 2;
      continue;
    }
    pos++; /* Skip comma */

    /* Parse data compression with bounds checking */
    if (pos < next_line)
    {
      int data_comp = 0;
      if (sscanf(pos, "%d", &data_comp) == 1)
      {
        if (data_comp >= 0 && data_comp < CGDCONT_DATA_COMP_MAP_SIZE)
        {
          ctx->data_comp             = (cgdcont_data_comp_t) data_comp;
          ctx->present.has_data_comp = true;
        }
      }
    }

    /* Skip to next field with bounds checking */
    while (*pos != ',' && pos < next_line)
    {
      pos++;
    }

    /* Check if we reached a comma or end of line */
    if (*pos != ',' || pos >= next_line)
    {
      read_data->num_contexts++;
      line_start = next_line + 2;
      continue;
    }
    pos++; /* Skip comma */

    /* Parse header compression with bounds checking */
    if (pos < next_line)
    {
      int head_comp = 0;
      if (sscanf(pos, "%d", &head_comp) == 1)
      {
        if (head_comp >= 0 && head_comp < CGDCONT_HEAD_COMP_MAP_SIZE)
        {
          ctx->head_comp             = (cgdcont_head_comp_t) head_comp;
          ctx->present.has_head_comp = true;
        }
      }
    }

    /* Skip to next field with bounds checking */
    while (*pos != ',' && pos < next_line)
    {
      pos++;
    }

    /* Process IPv4 address allocation if present */
    if (*pos == ',' && (pos + 1) < next_line)
    {
      pos++; /* Skip comma */

      /* Parse IPv4 address allocation with bounds checking */
      if (pos < next_line)
      {
        int ipv4_addr_alloc = 0;
        if (sscanf(pos, "%d", &ipv4_addr_alloc) == 1)
        {
          if (ipv4_addr_alloc >= 0 && ipv4_addr_alloc < CGDCONT_IPV4_ADDR_ALLOC_MAP_SIZE)
          {
            ctx->ipv4_addr_alloc             = (cgdcont_ipv4_addr_alloc_t) ipv4_addr_alloc;
            ctx->present.has_ipv4_addr_alloc = true;
          }
        }
      }
    }

    read_data->num_contexts++;

    /* Move to next line with safety check */
    if (next_line != NULL && next_line + 2 <= response + strlen(response))
    {
      line_start = next_line + 2;
    }
    else
    {
      break; /* End of string or no more complete lines */
    }
  }

  return ESP_OK;
}

// Formatter for write command
static esp_err_t cgdcont_write_formatter(const void* params, char* buffer, size_t buffer_size)
{
  if (!params || !buffer || buffer_size == 0)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  const cgdcont_write_params_t* write_params = (const cgdcont_write_params_t*) params;
  int                           written      = 0;

  // Validate CID range
  if (write_params->cid < CGDCONT_CID_RANGE_MIN_VALUE ||
      write_params->cid > CGDCONT_CID_RANGE_MAX_VALUE)
  {
    ESP_LOGE(TAG, "Invalid CID: %d (must be 1-15)", write_params->cid);
    return ESP_ERR_INVALID_ARG;
  }

  // If only CID is provided, format as "=<cid>" to reset the context
  if (!write_params->present.has_pdp_type)
  {
    written = snprintf(buffer, buffer_size, "=%d", write_params->cid);
  }
  else
  {
    // Get PDP type string
    const char* pdp_type_str = "IP"; // Default
    for (int i = 0; i < CGDCONT_PDP_TYPE_MAP_SIZE; i++)
    {
      if (write_params->pdp_type == CGDCONT_PDP_TYPE_MAP[i].value)
      {
        pdp_type_str = CGDCONT_PDP_TYPE_MAP[i].string;
        break;
      }
    }

    // Format with required fields: CID and PDP type
    written = snprintf(buffer, buffer_size, "=%d,\"%s\"", write_params->cid, pdp_type_str);

    // Add APN if present
    if (write_params->present.has_apn && written > 0 && (size_t) written < buffer_size)
    {
      int apn_written =
          snprintf(buffer + written, buffer_size - written, ",\"%s\"", write_params->apn);
      if (apn_written > 0)
      {
        written += apn_written;
      }
    }
    else if (written > 0 && (size_t) written < buffer_size)
    {
      // Empty APN field placeholder
      written += snprintf(buffer + written, buffer_size - written, ",\"\"");
    }

    // Add PDP address if present or APN is present
    if (write_params->present.has_pdp_addr && written > 0 && (size_t) written < buffer_size)
    {
      int addr_written =
          snprintf(buffer + written, buffer_size - written, ",\"%s\"", write_params->pdp_addr);
      if (addr_written > 0)
      {
        written += addr_written;
      }
    }
    else if (write_params->present.has_apn && written > 0 && (size_t) written < buffer_size)
    {
      // Empty PDP address field placeholder if APN was provided
      written += snprintf(buffer + written, buffer_size - written, ",\"\"");
    }

    // Add data compression if present or earlier fields were provided
    if (write_params->present.has_data_comp && written > 0 && (size_t) written < buffer_size)
    {
      int comp_written =
          snprintf(buffer + written, buffer_size - written, ",%d", write_params->data_comp);
      if (comp_written > 0)
      {
        written += comp_written;
      }
    }
    else if (write_params->present.has_pdp_addr && written > 0 && (size_t) written < buffer_size)
    {
      // Default data compression value
      written += snprintf(buffer + written, buffer_size - written, ",%d", CGDCONT_DATA_COMP_OFF);
    }

    // Add header compression if present or earlier fields were provided
    if (write_params->present.has_head_comp && written > 0 && (size_t) written < buffer_size)
    {
      int head_written =
          snprintf(buffer + written, buffer_size - written, ",%d", write_params->head_comp);
      if (head_written > 0)
      {
        written += head_written;
      }
    }
    else if (write_params->present.has_data_comp && written > 0 && (size_t) written < buffer_size)
    {
      // Default header compression value
      written += snprintf(buffer + written, buffer_size - written, ",%d", CGDCONT_HEAD_COMP_OFF);
    }

    // Add IPv4 address allocation if present or earlier fields were provided
    if (write_params->present.has_ipv4_addr_alloc && written > 0 && (size_t) written < buffer_size)
    {
      int ipv4_written =
          snprintf(buffer + written, buffer_size - written, ",%d", write_params->ipv4_addr_alloc);
      if (ipv4_written > 0)
      {
        written += ipv4_written;
      }
    }
  }

  // Check if buffer was too small
  if (written < 0 || (size_t) written >= buffer_size)
  {
    ESP_LOGE(TAG, "Buffer too small for formatting CGDCONT write command");
    if (buffer_size > 0)
    {
      buffer[0] = '\0'; // Ensure null-terminated in case of overflow
    }
    return ESP_ERR_INVALID_SIZE;
  }

  return ESP_OK;
}

// CGDCONT command definition
const at_cmd_t AT_CMD_CGDCONT = {
    .name        = "CGDCONT",
    .description = "Define PDP Context",
    .type_info   = {[AT_CMD_TYPE_TEST] = AT_CMD_TYPE_NOT_IMPLEMENTED,
                    // [AT_CMD_TYPE_TEST]    = {.parser = cgdcont_test_parser, .formatter = NULL},
                    [AT_CMD_TYPE_READ]    = {.parser        = cgdcont_read_parser,
                                             .formatter     = NULL,
                                             .response_type = AT_CMD_RESPONSE_TYPE_DATA_REQUIRED},
                    [AT_CMD_TYPE_WRITE]   = {.parser        = NULL,
                                             .formatter     = cgdcont_write_formatter,
                                             .response_type = AT_CMD_RESPONSE_TYPE_SIMPLE_ONLY},
                    [AT_CMD_TYPE_EXECUTE] = AT_CMD_TYPE_DOES_NOT_EXIST},
    .timeout_ms  = 300 // 300ms per spec
};
