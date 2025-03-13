#include "at_cmd_qmtcfg.h"

#include "at_cmd_structure.h"
#include "esp_err.h"
#include "esp_log.h"

#include <string.h>

static const char* TAG = "AT_CMD_QMTCFG";

// Define the mapping arrays
const enum_str_map_t QMTCFG_VERSION_MAP[QMTCFG_VERSION_MAP_SIZE] = {
    {QMTCFG_VERSION_MQTT_3_1, "MQTT v3.1"}, {QMTCFG_VERSION_MQTT_3_1_1, "MQTT v3.1.1"}};

const enum_str_map_t QMTCFG_SSL_MODE_MAP[QMTCFG_SSL_MODE_MAP_SIZE] = {
    {QMTCFG_SSL_DISABLE, "Normal TCP"}, {QMTCFG_SSL_ENABLE, "SSL TCP"}};

const enum_str_map_t QMTCFG_CLEAN_SESSION_MAP[QMTCFG_CLEAN_SESSION_MAP_SIZE] = {
    {QMTCFG_CLEAN_SESSION_DISABLE, "Store subscriptions"},
    {QMTCFG_CLEAN_SESSION_ENABLE, "Discard information"}};

const enum_str_map_t QMTCFG_WILL_FLAG_MAP[QMTCFG_WILL_FLAG_MAP_SIZE] = {
    {QMTCFG_WILL_FLAG_IGNORE, "Ignore"}, {QMTCFG_WILL_FLAG_REQUIRE, "Require"}};

const enum_str_map_t QMTCFG_WILL_QOS_MAP[QMTCFG_WILL_QOS_MAP_SIZE] = {
    {QMTCFG_WILL_QOS_0, "At most once"},
    {QMTCFG_WILL_QOS_1, "At least once"},
    {QMTCFG_WILL_QOS_2, "Exactly once"}};

const enum_str_map_t QMTCFG_WILL_RETAIN_MAP[QMTCFG_WILL_RETAIN_MAP_SIZE] = {
    {QMTCFG_WILL_RETAIN_DISABLE, "Don't retain"}, {QMTCFG_WILL_RETAIN_ENABLE, "Retain"}};

const enum_str_map_t QMTCFG_MSG_RECV_MODE_MAP[QMTCFG_MSG_RECV_MODE_MAP_SIZE] = {
    {QMTCFG_MSG_RECV_MODE_CONTAIN_IN_URC, "Contained in URC"},
    {QMTCFG_MSG_RECV_MODE_NOT_CONTAIN_IN_URC, "Not contained in URC"}};

const enum_str_map_t QMTCFG_MSG_LEN_ENABLE_MAP[QMTCFG_MSG_LEN_ENABLE_MAP_SIZE] = {
    {QMTCFG_MSG_LEN_DISABLE, "Length not contained"}, {QMTCFG_MSG_LEN_ENABLE, "Length contained"}};

const enum_str_map_t QMTCFG_TIMEOUT_NOTICE_MAP[QMTCFG_TIMEOUT_NOTICE_MAP_SIZE] = {
    {QMTCFG_TIMEOUT_NOTICE_DISABLE, "Do not report"}, {QMTCFG_TIMEOUT_NOTICE_ENABLE, "Report"}};

const enum_str_map_t QMTCFG_TYPE_MAP[QMTCFG_TYPE_MAP_SIZE] = {{QMTCFG_TYPE_VERSION, "version"},
                                                              {QMTCFG_TYPE_PDPCID, "pdpcid"},
                                                              {QMTCFG_TYPE_SSL, "ssl"},
                                                              {QMTCFG_TYPE_KEEPALIVE, "keepalive"},
                                                              {QMTCFG_TYPE_SESSION, "session"},
                                                              {QMTCFG_TYPE_TIMEOUT, "timeout"},
                                                              {QMTCFG_TYPE_WILL, "will"},
                                                              {QMTCFG_TYPE_RECV_MODE, "recv/mode"},
                                                              {QMTCFG_TYPE_ALIAUTH, "aliauth"}};

// Helper function to convert string to qmtcfg_type_t
static esp_err_t str_to_qmtcfg_type(const char* str, qmtcfg_type_t* type)
{
  if (NULL == str || NULL == type)
  {
    return ESP_ERR_INVALID_ARG;
  }

  for (size_t i = 0; i < QMTCFG_TYPE_MAP_SIZE; i++)
  {
    if (0 == strcmp(str, QMTCFG_TYPE_MAP[i].string))
    {
      *type = (qmtcfg_type_t) QMTCFG_TYPE_MAP[i].value;
      return ESP_OK;
    }
  }

  return ESP_ERR_NOT_FOUND;
}

static esp_err_t qmtcfg_test_parser(const char* response, void* parsed_data)
{
  if (NULL == response || NULL == parsed_data)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  qmtcfg_test_response_t* test_data = (qmtcfg_test_response_t*) parsed_data;

  // Initialize the structure
  (void) memset(test_data, 0, sizeof(qmtcfg_test_response_t));

  // Default values based on documentation
  test_data->client_idx_min                                             = 0;
  test_data->client_idx_max                                             = 5;
  test_data->supported_versions[0]                                      = QMTCFG_VERSION_MQTT_3_1;
  test_data->supported_versions[1]                                      = QMTCFG_VERSION_MQTT_3_1_1;
  test_data->num_supported_versions                                     = 2;
  test_data->pdp_cid_min                                                = 1;
  test_data->pdp_cid_max                                                = 16;
  test_data->supports_ssl_modes[QMTCFG_SSL_DISABLE]                     = true;
  test_data->supports_ssl_modes[QMTCFG_SSL_ENABLE]                      = true;
  test_data->ctx_index_min                                              = 0;
  test_data->ctx_index_max                                              = 5;
  test_data->keep_alive_min                                             = 0;
  test_data->keep_alive_max                                             = 3600;
  test_data->supports_clean_session_modes[QMTCFG_CLEAN_SESSION_DISABLE] = true;
  test_data->supports_clean_session_modes[QMTCFG_CLEAN_SESSION_ENABLE]  = true;
  test_data->pkt_timeout_min                                            = 1;
  test_data->pkt_timeout_max                                            = 60;
  test_data->retry_times_min                                            = 0;
  test_data->retry_times_max                                            = 10;
  test_data->supports_timeout_notice_modes[QMTCFG_TIMEOUT_NOTICE_DISABLE]     = true;
  test_data->supports_timeout_notice_modes[QMTCFG_TIMEOUT_NOTICE_ENABLE]      = true;
  test_data->supports_will_flags[QMTCFG_WILL_FLAG_IGNORE]                     = true;
  test_data->supports_will_flags[QMTCFG_WILL_FLAG_REQUIRE]                    = true;
  test_data->supports_will_qos[QMTCFG_WILL_QOS_0]                             = true;
  test_data->supports_will_qos[QMTCFG_WILL_QOS_1]                             = true;
  test_data->supports_will_qos[QMTCFG_WILL_QOS_2]                             = true;
  test_data->supports_will_retain[QMTCFG_WILL_RETAIN_DISABLE]                 = true;
  test_data->supports_will_retain[QMTCFG_WILL_RETAIN_ENABLE]                  = true;
  test_data->supports_msg_recv_modes[QMTCFG_MSG_RECV_MODE_CONTAIN_IN_URC]     = true;
  test_data->supports_msg_recv_modes[QMTCFG_MSG_RECV_MODE_NOT_CONTAIN_IN_URC] = true;
  test_data->supports_msg_len_modes[QMTCFG_MSG_LEN_DISABLE]                   = true;
  test_data->supports_msg_len_modes[QMTCFG_MSG_LEN_ENABLE]                    = true;

  // Find response lines
  const char* version_line   = strstr(response, "+QMTCFG: \"version\"");
  const char* pdpcid_line    = strstr(response, "+QMTCFG: \"pdpcid\"");
  const char* ssl_line       = strstr(response, "+QMTCFG: \"ssl\"");
  const char* keepalive_line = strstr(response, "+QMTCFG: \"keepalive\"");
  const char* session_line   = strstr(response, "+QMTCFG: \"session\"");
  const char* timeout_line   = strstr(response, "+QMTCFG: \"timeout\"");
  const char* will_line      = strstr(response, "+QMTCFG: \"will\"");
  const char* recv_mode_line = strstr(response, "+QMTCFG: \"recv/mode\"");

  // Parse version line
  if (version_line)
  {
    int min_idx = 0, max_idx = 0;
    if (sscanf(version_line, "+QMTCFG: \"version\",(%d-%d)", &min_idx, &max_idx) == 2)
    {
      test_data->client_idx_min = (uint8_t) min_idx;
      test_data->client_idx_max = (uint8_t) max_idx;
    }
  }

  // Parse pdpcid line
  if (pdpcid_line)
  {
    int min_idx = 0, max_idx = 0, min_cid = 0, max_cid = 0;
    if (sscanf(pdpcid_line,
               "+QMTCFG: \"pdpcid\",(%d-%d),(%d-%d)",
               &min_idx,
               &max_idx,
               &min_cid,
               &max_cid) == 4)
    {
      test_data->pdp_cid_min = (uint8_t) min_cid;
      test_data->pdp_cid_max = (uint8_t) max_cid;
    }
  }

  // Parse ssl line
  if (ssl_line)
  {
    int  min_idx = 0, max_idx = 0, min_ctx = 0, max_ctx = 0;
    char ssl_modes[32] = {0};
    if (sscanf(ssl_line,
               "+QMTCFG: \"ssl\",(%d-%d),(%[^)]),(%d-%d)",
               &min_idx,
               &max_idx,
               ssl_modes,
               &min_ctx,
               &max_ctx) == 5)
    {
      test_data->ctx_index_min = (uint8_t) min_ctx;
      test_data->ctx_index_max = (uint8_t) max_ctx;

      // Parse SSL modes
      test_data->supports_ssl_modes[QMTCFG_SSL_DISABLE] = (strchr(ssl_modes, '0') != NULL);
      test_data->supports_ssl_modes[QMTCFG_SSL_ENABLE]  = (strchr(ssl_modes, '1') != NULL);
    }
  }

  // Parse keepalive line
  if (keepalive_line)
  {
    int min_idx = 0, max_idx = 0, min_keep = 0, max_keep = 0;
    if (sscanf(keepalive_line,
               "+QMTCFG: \"keepalive\",(%d-%d),(%d-%d)",
               &min_idx,
               &max_idx,
               &min_keep,
               &max_keep) == 4)
    {
      test_data->keep_alive_min = (uint16_t) min_keep;
      test_data->keep_alive_max = (uint16_t) max_keep;
    }
  }

  // Parse session line
  if (session_line)
  {
    char clean_session_modes[32] = {0};
    if (strstr(session_line, "0") != NULL)
    {
      test_data->supports_clean_session_modes[QMTCFG_CLEAN_SESSION_DISABLE] = true;
    }
    if (strstr(session_line, "1") != NULL)
    {
      test_data->supports_clean_session_modes[QMTCFG_CLEAN_SESSION_ENABLE] = true;
    }
  }

  // Parse timeout line
  if (timeout_line)
  {
    int min_timeout = 0, max_timeout = 0, min_retry = 0, max_retry = 0;
    if (sscanf(timeout_line,
               "+QMTCFG: \"timeout\",(%*[^)]),(%d-%d),(%d-%d)",
               &min_timeout,
               &max_timeout,
               &min_retry,
               &max_retry) == 4)
    {
      test_data->pkt_timeout_min = (uint8_t) min_timeout;
      test_data->pkt_timeout_max = (uint8_t) max_timeout;
      test_data->retry_times_min = (uint8_t) min_retry;
      test_data->retry_times_max = (uint8_t) max_retry;
    }
  }

  // TODO: The rest of the parsing would follow the same pattern

  return ESP_OK;
}

static esp_err_t qmtcfg_write_parser(const char* response, void* parsed_data)
{
  if (NULL == response || NULL == parsed_data)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  qmtcfg_write_response_t* write_response = (qmtcfg_write_response_t*) parsed_data;

  // Find response start
  const char* data_start = strstr(response, "+QMTCFG: \"");
  if (NULL == data_start)
  {
    // No data response, only OK response
    // This typically means the command was a write that succeeded, not a query
    return ESP_OK;
  }

  data_start += 10; // Skip "+QMTCFG: \""

  // Extract type string
  char   type_str[32] = {0};
  size_t i            = 0;
  while (data_start[i] != '"' && data_start[i] != '\0' && i < sizeof(type_str) - 1)
  {
    type_str[i] = data_start[i];
    i++;
  }
  type_str[i] = '\0';

  // Check if we extracted a valid type string
  if (i == 0 || data_start[i] != '"')
  {
    ESP_LOGE(TAG, "Malformed response: missing or invalid type string");
    return ESP_ERR_INVALID_RESPONSE;
  }

  // Convert type string to enum
  qmtcfg_type_t config_type;
  if (str_to_qmtcfg_type(type_str, &config_type) != ESP_OK)
  {
    ESP_LOGE(TAG, "Invalid configuration type: %s", type_str);
    return ESP_ERR_INVALID_RESPONSE;
  }

  write_response->type = config_type;

  // Find parameters after the type
  const char* params_start = data_start + i + 2; // Skip type string and "\","
  if (*params_start != ',')
  {
    params_start--; // Adjust if there's no comma
  }

  // Parse parameters based on type
  switch (config_type)
  {
    case QMTCFG_TYPE_VERSION:
    {
      int version = 0;
      if (sscanf(params_start, ",%d", &version) == 1)
      {
        if (version == 3 || version == 4)
        {
          write_response->response.version.version             = (qmtcfg_version_t) version;
          write_response->response.version.present.has_version = true;
        }
      }
    }
    break;

    case QMTCFG_TYPE_PDPCID:
    {
      int pdp_cid = 0;
      if (sscanf(params_start, ",%d", &pdp_cid) == 1)
      {
        if (pdp_cid >= 1 && pdp_cid <= 16)
        {
          write_response->response.pdpcid.pdp_cid             = (uint8_t) pdp_cid;
          write_response->response.pdpcid.present.has_pdp_cid = true;
        }
      }
    }
    break;

    case QMTCFG_TYPE_SSL:
    {
      int ssl_enable = 0, ctx_index = 0;
      int items = sscanf(params_start, ",%d,%d", &ssl_enable, &ctx_index);

      if (items >= 1 && (ssl_enable == 0 || ssl_enable == 1))
      {
        write_response->response.ssl.ssl_enable             = (qmtcfg_ssl_mode_t) ssl_enable;
        write_response->response.ssl.present.has_ssl_enable = true;
      }

      if (items >= 2 && ctx_index >= 0 && ctx_index <= 5)
      {
        write_response->response.ssl.ctx_index             = (uint8_t) ctx_index;
        write_response->response.ssl.present.has_ctx_index = true;
      }
    }
    break;

    case QMTCFG_TYPE_KEEPALIVE:
    {
      int keep_alive_time = 0;
      if (sscanf(params_start, ",%d", &keep_alive_time) == 1)
      {
        if (keep_alive_time >= 0 && keep_alive_time <= 3600)
        {
          write_response->response.keepalive.keep_alive_time = (uint16_t) keep_alive_time;
          write_response->response.keepalive.present.has_keep_alive_time = true;
        }
      }
    }
    break;

    case QMTCFG_TYPE_SESSION:
    {
      int clean_session = 0;
      if (sscanf(params_start, ",%d", &clean_session) == 1)
      {
        if (clean_session == 0 || clean_session == 1)
        {
          write_response->response.session.clean_session = (qmtcfg_clean_session_t) clean_session;
          write_response->response.session.present.has_clean_session = true;
        }
      }
    }
    break;

    case QMTCFG_TYPE_TIMEOUT:
    {
      int pkt_timeout = 0, retry_times = 0, timeout_notice = 0;
      int items = sscanf(params_start, ",%d,%d,%d", &pkt_timeout, &retry_times, &timeout_notice);

      if (items >= 1 && pkt_timeout >= 1 && pkt_timeout <= 60)
      {
        write_response->response.timeout.pkt_timeout             = (uint8_t) pkt_timeout;
        write_response->response.timeout.present.has_pkt_timeout = true;
      }

      if (items >= 2 && retry_times >= 0 && retry_times <= 10)
      {
        write_response->response.timeout.retry_times             = (uint8_t) retry_times;
        write_response->response.timeout.present.has_retry_times = true;
      }

      if (items >= 3 && (timeout_notice == 0 || timeout_notice == 1))
      {
        write_response->response.timeout.timeout_notice = (qmtcfg_timeout_notice_t) timeout_notice;
        write_response->response.timeout.present.has_timeout_notice = true;
      }
    }
    break;

    case QMTCFG_TYPE_WILL:
    {
      int will_flag = 0, will_qos = 0, will_retain = 0;
      if (sscanf(params_start, ",%d", &will_flag) == 1)
      {
        write_response->response.will.will_flag             = (qmtcfg_will_flag_t) will_flag;
        write_response->response.will.present.has_will_flag = true;

        if (will_flag == 1)
        {
          // Find parameters for will
          const char* will_params = strchr(params_start + 1, ',');
          if (will_params)
          {
            if (sscanf(will_params, ",%d,%d", &will_qos, &will_retain) == 2)
            {
              if (will_qos >= 0 && will_qos <= 2)
              {
                write_response->response.will.will_qos             = (qmtcfg_will_qos_t) will_qos;
                write_response->response.will.present.has_will_qos = true;
              }

              if (will_retain == 0 || will_retain == 1)
              {
                write_response->response.will.will_retain = (qmtcfg_will_retain_t) will_retain;
                write_response->response.will.present.has_will_retain = true;
              }

              // Extract topic and message - these are quoted strings
              const char* topic_start = strstr(will_params, ",\"");
              if (topic_start)
              {
                topic_start += 2; // Skip ,\"
                const char* topic_end = strchr(topic_start, '"');
                if (topic_end)
                {
                  size_t topic_len = topic_end - topic_start;
                  if (topic_len < sizeof(write_response->response.will.will_topic))
                  {
                    strncpy(write_response->response.will.will_topic, topic_start, topic_len);
                    write_response->response.will.will_topic[topic_len]  = '\0';
                    write_response->response.will.present.has_will_topic = true;

                    // Now look for message
                    const char* msg_start = strstr(topic_end, ",\"");
                    if (msg_start)
                    {
                      msg_start += 2; // Skip ,\"
                      const char* msg_end = strchr(msg_start, '"');
                      if (msg_end)
                      {
                        size_t msg_len = msg_end - msg_start;
                        if (msg_len < sizeof(write_response->response.will.will_message))
                        {
                          strncpy(write_response->response.will.will_message, msg_start, msg_len);
                          write_response->response.will.will_message[msg_len]    = '\0';
                          write_response->response.will.present.has_will_message = true;
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    break;

    case QMTCFG_TYPE_RECV_MODE:
    {
      int msg_recv_mode = 0, msg_len_enable = 0;
      int items = sscanf(params_start, ",%d,%d", &msg_recv_mode, &msg_len_enable);

      if (items >= 1 && (msg_recv_mode == 0 || msg_recv_mode == 1))
      {
        write_response->response.recv_mode.msg_recv_mode = (qmtcfg_msg_recv_mode_t) msg_recv_mode;
        write_response->response.recv_mode.present.has_msg_recv_mode = true;
      }

      if (items >= 2 && (msg_len_enable == 0 || msg_len_enable == 1))
      {
        write_response->response.recv_mode.msg_len_enable =
            (qmtcfg_msg_len_enable_t) msg_len_enable;
        write_response->response.recv_mode.present.has_msg_len_enable = true;
      }
    }
    break;

    case QMTCFG_TYPE_ALIAUTH:
      // Aliauth response parsing is skipped as requested
      break;

    default:
      ESP_LOGW(TAG, "Unknown configuration type in response: %d", config_type);
      return ESP_ERR_INVALID_RESPONSE;
  }

  return ESP_OK;
}

static esp_err_t qmtcfg_write_formatter(const void* params, char* buffer, size_t buffer_size)
{
  if (NULL == params || NULL == buffer || 0 == buffer_size)
  {
    ESP_LOGE(TAG, "Invalid arguments");
    return ESP_ERR_INVALID_ARG;
  }

  const qmtcfg_write_params_t* write_params = (const qmtcfg_write_params_t*) params;
  int                          written      = 0;

  // Get the configuration type string
  const char* config_type_str = "unknown";
  for (size_t i = 0; i < QMTCFG_TYPE_MAP_SIZE; i++)
  {
    if (write_params->type == (qmtcfg_type_t) QMTCFG_TYPE_MAP[i].value)
    {
      config_type_str = QMTCFG_TYPE_MAP[i].string;
      break;
    }
  }

  // Format the command based on the configuration type
  switch (write_params->type)
  {
    case QMTCFG_TYPE_VERSION:
    {
      const qmtcfg_write_version_params_t* version_params = &write_params->params.version;

      // Validate client_idx range
      if (version_params->client_idx > 5)
      {
        ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-5)", version_params->client_idx);
        return ESP_ERR_INVALID_ARG;
      }

      if (version_params->present.has_version)
      {
        // Validate version value
        if (version_params->version != QMTCFG_VERSION_MQTT_3_1 &&
            version_params->version != QMTCFG_VERSION_MQTT_3_1_1)
        {
          ESP_LOGE(TAG, "Invalid MQTT version: %d", version_params->version);
          return ESP_ERR_INVALID_ARG;
        }

        written = snprintf(buffer,
                           buffer_size,
                           "=\"%s\",%d,%d",
                           config_type_str,
                           version_params->client_idx,
                           version_params->version);
      }
      else
      {
        written = snprintf(
            buffer, buffer_size, "=\"%s\",%d", config_type_str, version_params->client_idx);
      }
    }
    break;

    case QMTCFG_TYPE_PDPCID:
    {
      const qmtcfg_write_pdpcid_params_t* pdpcid_params = &write_params->params.pdpcid;

      // Validate client_idx range
      if (pdpcid_params->client_idx > 5)
      {
        ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-5)", pdpcid_params->client_idx);
        return ESP_ERR_INVALID_ARG;
      }

      if (pdpcid_params->present.has_pdp_cid)
      {
        // Validate pdp_cid range
        if (pdpcid_params->pdp_cid < 1 || pdpcid_params->pdp_cid > 16)
        {
          ESP_LOGE(TAG, "Invalid PDP CID: %d (must be 1-16)", pdpcid_params->pdp_cid);
          return ESP_ERR_INVALID_ARG;
        }

        written = snprintf(buffer,
                           buffer_size,
                           "=\"%s\",%d,%d",
                           config_type_str,
                           pdpcid_params->client_idx,
                           pdpcid_params->pdp_cid);
      }
      else
      {
        written =
            snprintf(buffer, buffer_size, "=\"%s\",%d", config_type_str, pdpcid_params->client_idx);
      }
    }
    break;

    case QMTCFG_TYPE_SSL:
    {
      const qmtcfg_write_ssl_params_t* ssl_params = &write_params->params.ssl;

      // Validate client_idx range
      if (ssl_params->client_idx > 5)
      {
        ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-5)", ssl_params->client_idx);
        return ESP_ERR_INVALID_ARG;
      }

      if (ssl_params->present.has_ssl_enable)
      {
        // Validate ssl_enable value
        if (ssl_params->ssl_enable != QMTCFG_SSL_DISABLE &&
            ssl_params->ssl_enable != QMTCFG_SSL_ENABLE)
        {
          ESP_LOGE(TAG, "Invalid SSL enable value: %d", ssl_params->ssl_enable);
          return ESP_ERR_INVALID_ARG;
        }

        if (ssl_params->present.has_ctx_index)
        {
          // Validate ctx_index range
          if (ssl_params->ctx_index > 5)
          {
            ESP_LOGE(TAG, "Invalid SSL context index: %d (must be 0-5)", ssl_params->ctx_index);
            return ESP_ERR_INVALID_ARG;
          }

          written = snprintf(buffer,
                             buffer_size,
                             "=\"%s\",%d,%d,%d",
                             config_type_str,
                             ssl_params->client_idx,
                             ssl_params->ssl_enable,
                             ssl_params->ctx_index);
        }
        else
        {
          written = snprintf(buffer,
                             buffer_size,
                             "=\"%s\",%d,%d",
                             config_type_str,
                             ssl_params->client_idx,
                             ssl_params->ssl_enable);
        }
      }
      else
      {
        written =
            snprintf(buffer, buffer_size, "=\"%s\",%d", config_type_str, ssl_params->client_idx);
      }
    }
    break;

    case QMTCFG_TYPE_KEEPALIVE:
    {
      const qmtcfg_write_keepalive_params_t* keepalive_params = &write_params->params.keepalive;

      // Validate client_idx range
      if (keepalive_params->client_idx > 5)
      {
        ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-5)", keepalive_params->client_idx);
        return ESP_ERR_INVALID_ARG;
      }

      if (keepalive_params->present.has_keep_alive_time)
      {
        // Validate keep_alive_time range
        if (keepalive_params->keep_alive_time > 3600)
        {
          ESP_LOGE(TAG,
                   "Invalid keep-alive time: %d (must be 0-3600)",
                   keepalive_params->keep_alive_time);
          return ESP_ERR_INVALID_ARG;
        }

        written = snprintf(buffer,
                           buffer_size,
                           "=\"%s\",%d,%d",
                           config_type_str,
                           keepalive_params->client_idx,
                           keepalive_params->keep_alive_time);
      }
      else
      {
        written = snprintf(
            buffer, buffer_size, "=\"%s\",%d", config_type_str, keepalive_params->client_idx);
      }
    }
    break;

    case QMTCFG_TYPE_SESSION:
    {
      const qmtcfg_write_session_params_t* session_params = &write_params->params.session;

      // Validate client_idx range
      if (session_params->client_idx > 5)
      {
        ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-5)", session_params->client_idx);
        return ESP_ERR_INVALID_ARG;
      }

      if (session_params->present.has_clean_session)
      {
        // Validate clean_session value
        if (session_params->clean_session != QMTCFG_CLEAN_SESSION_DISABLE &&
            session_params->clean_session != QMTCFG_CLEAN_SESSION_ENABLE)
        {
          ESP_LOGE(TAG, "Invalid clean session value: %d", session_params->clean_session);
          return ESP_ERR_INVALID_ARG;
        }

        written = snprintf(buffer,
                           buffer_size,
                           "=\"%s\",%d,%d",
                           config_type_str,
                           session_params->client_idx,
                           session_params->clean_session);
      }
      else
      {
        written = snprintf(
            buffer, buffer_size, "=\"%s\",%d", config_type_str, session_params->client_idx);
      }
    }
    break;

    case QMTCFG_TYPE_TIMEOUT:
    {
      const qmtcfg_write_timeout_params_t* timeout_params = &write_params->params.timeout;

      // Validate client_idx range
      if (timeout_params->client_idx > 5)
      {
        ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-5)", timeout_params->client_idx);
        return ESP_ERR_INVALID_ARG;
      }

      if (timeout_params->present.has_pkt_timeout)
      {
        // Validate pkt_timeout range
        if (timeout_params->pkt_timeout < 1 || timeout_params->pkt_timeout > 60)
        {
          ESP_LOGE(TAG, "Invalid packet timeout: %d (must be 1-60)", timeout_params->pkt_timeout);
          return ESP_ERR_INVALID_ARG;
        }

        if (timeout_params->present.has_retry_times)
        {
          // Validate retry_times range
          if (timeout_params->retry_times > 10)
          {
            ESP_LOGE(TAG, "Invalid retry times: %d (must be 0-10)", timeout_params->retry_times);
            return ESP_ERR_INVALID_ARG;
          }

          if (timeout_params->present.has_timeout_notice)
          {
            // Validate timeout_notice value
            if (timeout_params->timeout_notice != QMTCFG_TIMEOUT_NOTICE_DISABLE &&
                timeout_params->timeout_notice != QMTCFG_TIMEOUT_NOTICE_ENABLE)
            {
              ESP_LOGE(TAG, "Invalid timeout notice value: %d", timeout_params->timeout_notice);
              return ESP_ERR_INVALID_ARG;
            }

            written = snprintf(buffer,
                               buffer_size,
                               "=\"%s\",%d,%d,%d,%d",
                               config_type_str,
                               timeout_params->client_idx,
                               timeout_params->pkt_timeout,
                               timeout_params->retry_times,
                               timeout_params->timeout_notice);
          }
          else
          {
            written = snprintf(buffer,
                               buffer_size,
                               "=\"%s\",%d,%d,%d",
                               config_type_str,
                               timeout_params->client_idx,
                               timeout_params->pkt_timeout,
                               timeout_params->retry_times);
          }
        }
        else
        {
          written = snprintf(buffer,
                             buffer_size,
                             "=\"%s\",%d,%d",
                             config_type_str,
                             timeout_params->client_idx,
                             timeout_params->pkt_timeout);
        }
      }
      else
      {
        written = snprintf(
            buffer, buffer_size, "=\"%s\",%d", config_type_str, timeout_params->client_idx);
      }
    }
    break;

    case QMTCFG_TYPE_WILL:
    {
      const qmtcfg_write_will_params_t* will_params = &write_params->params.will;

      // Validate client_idx range
      if (will_params->client_idx > 5)
      {
        ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-5)", will_params->client_idx);
        return ESP_ERR_INVALID_ARG;
      }

      if (will_params->present.has_will_flag)
      {
        // Validate will_flag value
        if (will_params->will_flag != QMTCFG_WILL_FLAG_IGNORE &&
            will_params->will_flag != QMTCFG_WILL_FLAG_REQUIRE)
        {
          ESP_LOGE(TAG, "Invalid will flag value: %d", will_params->will_flag);
          return ESP_ERR_INVALID_ARG;
        }

        if (will_params->will_flag == QMTCFG_WILL_FLAG_REQUIRE)
        {
          // If will is required, additional parameters must be present
          if (!will_params->present.has_will_qos || !will_params->present.has_will_retain ||
              !will_params->present.has_will_topic || !will_params->present.has_will_message)
          {
            ESP_LOGE(TAG, "All will parameters are required when will flag is set to require");
            return ESP_ERR_INVALID_ARG;
          }

          // Validate will_qos range
          if (will_params->will_qos > QMTCFG_WILL_QOS_2)
          {
            ESP_LOGE(TAG, "Invalid will QoS: %d (must be 0-2)", will_params->will_qos);
            return ESP_ERR_INVALID_ARG;
          }

          // Validate will_retain value
          if (will_params->will_retain != QMTCFG_WILL_RETAIN_DISABLE &&
              will_params->will_retain != QMTCFG_WILL_RETAIN_ENABLE)
          {
            ESP_LOGE(TAG, "Invalid will retain value: %d", will_params->will_retain);
            return ESP_ERR_INVALID_ARG;
          }

          written = snprintf(buffer,
                             buffer_size,
                             "=\"%s\",%d,%d,%d,%d,\"%s\",\"%s\"",
                             config_type_str,
                             will_params->client_idx,
                             will_params->will_flag,
                             will_params->will_qos,
                             will_params->will_retain,
                             will_params->will_topic,
                             will_params->will_message);
        }
        else
        {
          written = snprintf(buffer,
                             buffer_size,
                             "=\"%s\",%d,%d",
                             config_type_str,
                             will_params->client_idx,
                             will_params->will_flag);
        }
      }
      else
      {
        written =
            snprintf(buffer, buffer_size, "=\"%s\",%d", config_type_str, will_params->client_idx);
      }
    }
    break;

    case QMTCFG_TYPE_RECV_MODE:
    {
      const qmtcfg_write_recv_mode_params_t* recv_mode_params = &write_params->params.recv_mode;

      // Validate client_idx range
      if (recv_mode_params->client_idx > 5)
      {
        ESP_LOGE(TAG, "Invalid client_idx: %d (must be 0-5)", recv_mode_params->client_idx);
        return ESP_ERR_INVALID_ARG;
      }

      if (recv_mode_params->present.has_msg_recv_mode)
      {
        // Validate msg_recv_mode value
        if (recv_mode_params->msg_recv_mode != QMTCFG_MSG_RECV_MODE_CONTAIN_IN_URC &&
            recv_mode_params->msg_recv_mode != QMTCFG_MSG_RECV_MODE_NOT_CONTAIN_IN_URC)
        {
          ESP_LOGE(TAG, "Invalid message receive mode: %d", recv_mode_params->msg_recv_mode);
          return ESP_ERR_INVALID_ARG;
        }

        if (recv_mode_params->present.has_msg_len_enable)
        {
          // Validate msg_len_enable value
          if (recv_mode_params->msg_len_enable != QMTCFG_MSG_LEN_DISABLE &&
              recv_mode_params->msg_len_enable != QMTCFG_MSG_LEN_ENABLE)
          {
            ESP_LOGE(
                TAG, "Invalid message length enable value: %d", recv_mode_params->msg_len_enable);
            return ESP_ERR_INVALID_ARG;
          }

          written = snprintf(buffer,
                             buffer_size,
                             "=\"%s\",%d,%d,%d",
                             config_type_str,
                             recv_mode_params->client_idx,
                             recv_mode_params->msg_recv_mode,
                             recv_mode_params->msg_len_enable);
        }
        else
        {
          written = snprintf(buffer,
                             buffer_size,
                             "=\"%s\",%d,%d",
                             config_type_str,
                             recv_mode_params->client_idx,
                             recv_mode_params->msg_recv_mode);
        }
      }
      else
      {
        written = snprintf(
            buffer, buffer_size, "=\"%s\",%d", config_type_str, recv_mode_params->client_idx);
      }
    }
    break;

    case QMTCFG_TYPE_ALIAUTH:
      // Aliauth configuration formatting is skipped as requested
      written = snprintf(buffer,
                         buffer_size,
                         "=\"%s\",%d",
                         config_type_str,
                         0); // Default client index
      break;

    default:
      ESP_LOGE(TAG, "Unknown configuration type: %d", write_params->type);
      return ESP_ERR_INVALID_ARG;
  }

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

// AT command definition (at the bottom of the file as requested)
const at_cmd_t AT_CMD_QMTCFG = {
    .name        = "QMTCFG",
    .description = "Configure Optional Parameters of MQTT",
    .type_info   = {[AT_CMD_TYPE_TEST]    = {.parser = qmtcfg_test_parser, .formatter = NULL},
                    [AT_CMD_TYPE_READ]    = AT_CMD_TYPE_NOT_IMPLEMENTED,
                    [AT_CMD_TYPE_WRITE]   = {.parser    = qmtcfg_write_parser,
                                             .formatter = qmtcfg_write_formatter},
                    [AT_CMD_TYPE_EXECUTE] = AT_CMD_TYPE_NOT_IMPLEMENTED},
    .timeout_ms  = 3000 // NOTE: 300ms per spec BUT I added more time just to be safe
};
