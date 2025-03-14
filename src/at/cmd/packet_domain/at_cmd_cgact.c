#include "at_cmd_cgact.h"

#include "enum_utils.h"
#include "esp_log.h"

#include <string.h>

static const char* TAG = "AT_CMD_CGACT";

const enum_str_map_t CGACT_STATE_MAP[CGACT_STATE_MAP_SIZE] = {
    {CGACT_STATE_DEACTIVATED, "Deactivated"}, {CGACT_STATE_ACTIVATED, "Activated"}};

static esp_err_t cgact_read_parser(const char* response, void* parsed_data)
{
  /* Validate input parameters */
  if (NULL == response || NULL == parsed_data)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  cgact_read_response_t* read_data = (cgact_read_response_t*) parsed_data;

  /* Initialize output structure */
  (void) memset(read_data, 0, sizeof(cgact_read_response_t));

  const char* line_start = response;
  const char* next_line  = NULL;

  /* Process each line in response */
  while ((line_start = strstr(line_start, "+CGACT: ")) != NULL &&
         read_data->num_contexts < CGACT_MAX_NUM_CONTEXTS) /* Bounds check for maximum contexts */
  {
    line_start += 8; /* Skip "+CGACT: " */

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

    /* Parse CID and state values */
    int cid   = 0;
    int state = 0;

    if (sscanf(line_start, "%d,%d", &cid, &state) != 2)
    {
      /* Failed to parse both values, skip to next line */
      line_start = next_line + 2;
      continue;
    }

    /* Validate CID and state ranges */
    if (cid < 0 || cid >= CGACT_MAX_NUM_CONTEXTS || state < 0 || state > 1)
    {
      line_start = next_line + 2;
      continue;
    }

    /* Store the parsed values */
    cgact_context_t* ctx = &read_data->contexts[read_data->num_contexts];
    ctx->cid             = cid;
    ctx->state           = (cgact_state_t) state;
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

static esp_err_t cgact_write_formatter(const void* params, char* buffer, size_t buffer_size)
{
  if (!params || !buffer || buffer_size == 0)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  const cgact_write_params_t* write_params = (const cgact_write_params_t*) params;
  int                         written      = 0;

  // Validate provided param value range
  if (write_params->state < 0 || write_params->state > 1 || write_params->cid < 0 ||
      write_params->cid > CGACT_MAX_NUM_CONTEXTS)
  {
    return ESP_ERR_INVALID_ARG;
  }

  written = snprintf(buffer, buffer_size, "=%d,%d", write_params->state, write_params->cid);

  // Ensure buffer always in good state, even if overflow
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

const at_cmd_t AT_CMD_CGACT = {
    .name        = "CGACT",
    .description = "Activate or Deactivate specified PDP context",
    .type_info   = {[AT_CMD_TYPE_TEST]    = AT_CMD_TYPE_NOT_IMPLEMENTED,
                    [AT_CMD_TYPE_READ]    = {.parser = cgact_read_parser, .formatter = NULL},
                    [AT_CMD_TYPE_WRITE]   = {.parser = NULL, .formatter = cgact_write_formatter},
                    [AT_CMD_TYPE_EXECUTE] = AT_CMD_TYPE_DOES_NOT_EXIST},
    .timeout_ms  = 20000 // NOTE: Spec says 150 s , but this is too long for us to wait
};
