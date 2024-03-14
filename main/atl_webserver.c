/**
 * @file atl_webserver.c
 * @author Robson Costa (robson.costa@ifsc.edu.br)
 * @brief Webserver function.
 * @version 0.1.0
 * @date 2024-03-11 (created)
 * @date 2024-03-11 (updated)
 * 
 * @copyright Copyright &copy; since 2024 <a href="https://agrotechlab.lages.ifsc.edu.br" target="_blank">AgroTechLab</a>.\n
 * ![LICENSE license](../figs/license.png)<br>
 * Licensed under the CC BY-SA (<i>Creative Commons Attribution-ShareAlike</i>) 4.0 International Unported License (the <em>"License"</em>). You may not
 * use this file except in compliance with the License. You may obtain a copy of the License <a href="https://creativecommons.org/licenses/by-sa/4.0/legalcode" target="_blank">here</a>.
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an <em>"as is" basis, 
 * without warranties or conditions of any kind</em>, either express or implied. See the License for the specific language governing permissions 
 * and limitations under the License.
 */
#include <esp_err.h>
#include <esp_log.h>
#include <esp_https_server.h>
#include <esp_tls_crypto.h>
#include <esp_ota_ops.h>
#include <esp_image_format.h>
#include <esp_mac.h>
#include <cJSON.h>
#include "atl_webserver.h"
#include "atl_config.h"
#include "atl_led.h"

/* Constants */
static const char *TAG = "atl-webserver";
extern const char favicon_start[] asm("_binary_favicon_ico_start");
extern const char favicon_end[] asm("_binary_favicon_ico_end");
extern const char css_start[] asm("_binary_agrotechlab_css_start");
extern const char css_end[] asm("_binary_agrotechlab_css_end");
extern const char js_start[] asm("_binary_agrotechlab_js_start");
extern const char js_end[] asm("_binary_agrotechlab_js_end");
extern const char header_start[] asm("_binary_header_html_start");
extern const char header_end[] asm("_binary_header_html_end");
extern const char footer_start[] asm("_binary_footer_html_start");
extern const char footer_end[] asm("_binary_footer_html_end");
extern const unsigned char servercert_start[] asm("_binary_cacert_pem_start");
extern const unsigned char servercert_end[] asm("_binary_cacert_pem_end");
extern const unsigned char prvtkey_pem_start[] asm("_binary_prvtkey_pem_start");
extern const unsigned char prvtkey_pem_end[] asm("_binary_prvtkey_pem_end");

/* Global external variables */
extern atl_config_t atl_config;

/**
 * @fn favicon_get_handler(httpd_req_t *req)
 * @brief GET handler for FAVICON file
 * @details HTTP GET Handler for FAVICON file
 * @param[in] req - request
 * @return ESP error code
 */
static esp_err_t favicon_get_handler(httpd_req_t *req) {
    const uint32_t favicon_len = favicon_end - favicon_start;

    /* Reply favicon.ico */
    ESP_LOGD(TAG, "Sending favicon.ico");
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, favicon_start, favicon_len);

    return ESP_OK;
}

/**
 * @brief HTTP GET Handler for FAVICON file
 */
static const httpd_uri_t favicon = {
    .uri = "/favicon.ico",
    .method = HTTP_GET,
    .handler = favicon_get_handler
};

/**
 * @fn css_get_handler(httpd_req_t *req)
 * @brief GET handler for CSS file
 * @details HTTP GET Handler for CSS file
 * @param[in] req - request
 * @return ESP error code
 */
static esp_err_t css_get_handler(httpd_req_t *req) {
    const uint32_t css_len = css_end - css_start;

    /* Reply CSS */
    ESP_LOGD(TAG, "Sending agrotechlab.css");
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, css_start, css_len);

    return ESP_OK;
}

/**
 * @brief HTTP GET Handler for CSS file
 */
static const httpd_uri_t css = {
    .uri = "/agrotechlab.css",
    .method = HTTP_GET,
    .handler = css_get_handler
};

/**
 * @fn js_get_handler(httpd_req_t *req)
 * @brief GET handler for JS file
 * @details HTTP GET Handler for JS file
 * @param[in] req - request
 * @return ESP error code
 */
static esp_err_t js_get_handler(httpd_req_t *req) {
    const uint32_t js_len = js_end - js_start;

    /* Reply JavaScript */
    ESP_LOGD(TAG, "Sending agrotechlab.js");
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, js_start, js_len);

    return ESP_OK;
}

/**
 * @brief HTTP GET Handler for JS file
 */
static const httpd_uri_t js = {
    .uri = "/agrotechlab.js",
    .method = HTTP_GET,
    .handler = js_get_handler
};

/**
 * @fn http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
 * @brief 404 error handler
 * @details HTTP Error (404) Handler - Redirects all requests to the root page
 * @param[in] req - request
 * @param[in] err - HTTP error code
 * @return ESP error code
 */
static esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err) {
    
    /* Set status */
    httpd_resp_set_status(req, "302 Temporary Redirect");

    /* Redirect to the root directory */
    httpd_resp_set_hdr(req, "Location", "/index.html");

    /* iOS requires content in the response to detect a captive portal, simply redirecting is not sufficient. */
    httpd_resp_send(req, "Redirect to the home portal", HTTPD_RESP_USE_STRLEN);

    ESP_LOGW(TAG, "Redirecting request to root page!");
    return ESP_OK;
}

/**
 * @fn home_get_handler(httpd_req_t *req)
 * @brief GET handler for home webpage
 * @details HTTP GET Handler for home webpage
 * @param[in] req - request
 * @return ESP error code
*/
static esp_err_t home_get_handler(httpd_req_t *req) {
    ESP_LOGD(TAG, "Sending index.html");

    /* Set response status, type and header */
    httpd_resp_set_status(req, HTTPD_200);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Connection", "keep-alive");

    /* Send header chunck */
    const size_t header_size = (header_end - header_start);
    httpd_resp_send_chunk(req, (const char *)header_start, header_size);

    /* Send article chunks */
    httpd_resp_sendstr_chunk(req, "<p style=\"text-align:center\">Welcome to GreenField, an open hardware and open source weather station developed by ");
    httpd_resp_sendstr_chunk(req, "<a href=\"https://agrotechlab.lages.ifsc.edu.br\" target=\"_blank\">AgroTechLab</a>.</p>");

    /* Send footer chunck */
    const size_t footer_size = (footer_end - footer_start);
    httpd_resp_send_chunk(req, (const char *)footer_start, footer_size);

    /* Send empty chunk to signal HTTP response completion */
    httpd_resp_sendstr_chunk(req, NULL);

    return ESP_OK;
}

/**
 * @brief HTTP GET Handler for home webpage
 */
static const httpd_uri_t home_get = {
    .uri = "/index.html",
    .method = HTTP_GET,
    .handler = home_get_handler
};

/**
 * @fn conf_wifi_get_handler(httpd_req_t *req)
 * @brief GET handler for WiFi configuration webpage
 * @details HTTP GET handler for WiFi configuration webpage
 * @param[in] req - request
 * @return ESP error code
 */
static esp_err_t conf_wifi_get_handler(httpd_req_t *req) {
    char resp_val[65];
    ESP_LOGD(TAG, "Sending conf_wifi.html");

    /* Set response status, type and header */
    httpd_resp_set_status(req, HTTPD_200);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Connection", "keep-alive");

    /* Send header chunck */
    const size_t header_size = (header_end - header_start);
    httpd_resp_send_chunk(req, (const char *)header_start, header_size);

    /* Make a local copy of WIFI configuration */
    atl_config_wifi_t wifi_config;
    memset(&wifi_config, 0, sizeof(atl_config_wifi_t));
    if (xSemaphoreTake(atl_config_mutex, portMAX_DELAY) == pdTRUE) {
        memcpy(&wifi_config, &atl_config.wifi, sizeof(atl_config_wifi_t));
        xSemaphoreGive(atl_config_mutex);
    }
    else {
        ESP_LOGW(TAG, "Fail to get configuration mutex!");
    }

    /* Send article chunks */    
    httpd_resp_sendstr_chunk(req, "<form action=\"conf_wifi_post.html\" method=\"post\"> \
                                      <div class=\"row\"> \
                                      <table><tr><th>Parameter</th><th>Value</th></tr> \
                                      <tr><td>MAC Address</td><td>");
    uint8_t mac_addr[6] = {0};
    esp_efuse_mac_get_default((uint8_t*)&mac_addr);
    if (wifi_config.mode == ATL_WIFI_AP_MODE) {
        mac_addr[5]++;
    } 
    sprintf(resp_val, "%02X:%02X:%02X:%02X:%02X:%02X", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    httpd_resp_sendstr_chunk(req, resp_val);
    httpd_resp_sendstr_chunk(req,"</td></tr><tr><td>WiFi mode</td><td><select name=\"wifi_mode\" id=\"wifi_mode\">");
    if (wifi_config.mode == ATL_WIFI_AP_MODE) {
        httpd_resp_sendstr_chunk(req, "<option selected value=\"AP_MODE\">Access Point</option> \
                                       <option value=\"STA_MODE\">Station</option> \
                                       </select></td></tr>");
    } else if (wifi_config.mode == ATL_WIFI_STA_MODE) {
        httpd_resp_sendstr_chunk(req, "<option value=\"AP_MODE\">Access Point</option> \
                                       <option selected value=\"STA_MODE\">Station</option> \
                                       </select></td></tr>");
    }
    
    /* Process station BSSID name */
    httpd_resp_sendstr_chunk(req, "<tr><td><label for=\"bssid\">Network (BSSID):</label></td> \
                                    <td><input type=\"text\" id=\"bssid\" name=\"bssid\" value=\"");        
    sprintf(resp_val, "%s", wifi_config.sta_ssid);
    httpd_resp_sendstr_chunk(req, resp_val);
    httpd_resp_sendstr_chunk(req, "\"></td></tr>");

    /* Process station BSSID password */
    httpd_resp_sendstr_chunk(req, "<tr><td><label for=\"pass\">Password:</label></td> \
                                   <td><input type=\"password\" id=\"pass\" name=\"pass\" value=\"");           
    sprintf(resp_val, "%s", wifi_config.sta_pass);          
    httpd_resp_sendstr_chunk(req, resp_val);
    httpd_resp_sendstr_chunk(req, "\"></td></tr></table><br><div class=\"reboot-msg\" id=\"delayMsg\"></div>");

    /* Send button chunks */
    httpd_resp_sendstr_chunk(req, "<br><input class=\"btn_generic\" name=\"btn_save_reboot\" type=\"submit\" \
                                    onclick=\"delayRedirect()\" value=\"Save & Reboot\"></div></form>");     

    /* Send footer chunck */
    const size_t footer_size = (footer_end - footer_start);
    httpd_resp_send_chunk(req, (const char *)footer_start, footer_size);

    /* Send empty chunk to signal HTTP response completion */
    httpd_resp_sendstr_chunk(req, NULL);

    return ESP_OK;
}

/**
 * @fn conf_mqtt_get_handler(httpd_req_t *req)
 * @brief GET handler for MQTT Client configuration webpage
 * @details HTTP GET handler for MQTT Client configuration webpage
 * @param[in] req - request
 * @return ESP error code
*/
static esp_err_t conf_mqtt_get_handler(httpd_req_t *req) {
    char resp_val[65];
    ESP_LOGD(TAG, "Sending conf_mqtt.html");

    /* Set response status, type and header */
    httpd_resp_set_status(req, HTTPD_200);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Connection", "keep-alive");

    /* Send header chunck */
    const size_t header_size = (header_end - header_start);
    httpd_resp_send_chunk(req, (const char *)header_start, header_size);

    /* Make a local copy of MQTT client configuration */
    atl_mqtt_client_t mqtt_client_config;
    memset(&mqtt_client_config, 0, sizeof(atl_mqtt_client_t));
    if (xSemaphoreTake(atl_config_mutex, portMAX_DELAY) == pdTRUE) {
        memcpy(&mqtt_client_config, &atl_config.mqtt_client, sizeof(atl_mqtt_client_t));
        xSemaphoreGive(atl_config_mutex);
    }
    else {
        ESP_LOGW(TAG, "Fail to get configuration mutex!");
    }
    
    /* Send article chunks */    
    httpd_resp_sendstr_chunk(req, "<form action=\"conf_mqtt_post.html\" method=\"post\"><div class=\"row\"> \
                                      <table><tr><th>Parameter</th><th>Value</th></tr> \
                                      <tr><td>MQTT Mode</td><td><select name=\"mqtt_mode\" id=\"mqtt_mode\">");
    if (mqtt_client_config.mode == ATL_MQTT_DISABLED) {
        httpd_resp_sendstr_chunk(req, "<option selected value=\"ATL_MQTT_DISABLED\">MQTT Client Disabled</option> \
                                       <option value=\"ATL_MQTT_AGROTECHLAB_CLOUD\">AgroTechLab Cloud</option> \
                                       <option value=\"ATL_MQTT_THIRD\">Third Server</option> \
                                       </select></td></tr>");
    } else if (mqtt_client_config.mode == ATL_MQTT_AGROTECHLAB_CLOUD) {
        httpd_resp_sendstr_chunk(req, "<option value=\"ATL_MQTT_DISABLED\">MQTT Client Disabled</option> \
                                       <option selected value=\"ATL_MQTT_AGROTECHLAB_CLOUD\">AgroTechLab Cloud</option> \
                                       <option value=\"ATL_MQTT_THIRD\">Third Server</option> \
                                       </select></td></tr>");
    } else if (mqtt_client_config.mode == ATL_MQTT_THIRD) {
        httpd_resp_sendstr_chunk(req, "<option value=\"ATL_MQTT_DISABLED\">MQTT Client Disabled</option> \
                                       <option value=\"ATL_MQTT_AGROTECHLAB_CLOUD\">AgroTechLab Cloud</option> \
                                       <option selected value=\"ATL_MQTT_THIRD\">Third Server</option> \
                                       </select></td></tr>");
    }    
    httpd_resp_sendstr_chunk(req, "<tr><td>MQTT Server Address</td><td><input type=\"text\" id=\"mqtt_srv_addr\" name=\"mqtt_srv_addr\" value=\"");
    sprintf(resp_val, "%s", mqtt_client_config.broker_address);
    httpd_resp_sendstr_chunk(req, resp_val);
    httpd_resp_sendstr_chunk(req, "\"></td></tr><tr><td>MQTT Server Port</td><td><input type=\"number\" id=\"mqtt_srv_port\" name=\"mqtt_srv_port\" value=\"");
    sprintf(resp_val, "%d", mqtt_client_config.broker_port);
    httpd_resp_sendstr_chunk(req, resp_val);
    httpd_resp_sendstr_chunk(req, "\"></td></tr><tr><td>Transport</td><td><select name=\"mqtt_transport\" id=\"mqtt_transport\">");
    if (mqtt_client_config.transport == MQTT_TRANSPORT_OVER_TCP) {
        httpd_resp_sendstr_chunk(req, "<option selected value=\"MQTT_TRANSPORT_OVER_TCP\">MQTT (TCP)</option> \
                                       <option value=\"MQTT_TRANSPORT_OVER_SSL\">MQTTS (TCP+TLS)</option></select></td></tr>");
    } else if (mqtt_client_config.transport == MQTT_TRANSPORT_OVER_SSL) {
        httpd_resp_sendstr_chunk(req, "<option value=\"MQTT_TRANSPORT_OVER_TCP\">MQTT (TCP)</option> \
                                       <option selected value=\"MQTT_TRANSPORT_OVER_SSL\">MQTTS (TCP+TLS)</option></select></td></tr>");
    }
    httpd_resp_sendstr_chunk(req, "<tr><td>Disable Common Name (CN) check</td><td><select name=\"mqtt_disable_cn_check\" id=\"mqtt_disable_cn_check\">");
    if (mqtt_client_config.disable_cn_check == true) {
        httpd_resp_sendstr_chunk(req, "<option selected value=\"true\">true</option> \
                                       <option value=\"false\">false</option></select></td></tr>");
    } else {
        httpd_resp_sendstr_chunk(req, "<option value=\"true\">true</option> \
                                       <option selected value=\"false\">false</option></select></td></tr>");
    }    
    httpd_resp_sendstr_chunk(req, "<tr><td>Username</td><td><input type=\"text\" id=\"mqtt_username\" name=\"mqtt_username\" value=\"");
    sprintf(resp_val, "%s", mqtt_client_config.user);
    httpd_resp_sendstr_chunk(req, resp_val);
    httpd_resp_sendstr_chunk(req, "\"></td></tr><tr><td>Password</td><td><input type=\"password\" id=\"mqtt_pass\" name=\"mqtt_pass\" value=\"");
    sprintf(resp_val, "%s", mqtt_client_config.pass);
    httpd_resp_sendstr_chunk(req, resp_val);
    httpd_resp_sendstr_chunk(req, "\"></td></tr><tr><td>QoS</td><td><select name=\"mqtt_qos\" id=\"mqtt_qos\">");
    if (mqtt_client_config.qos == ATL_MQTT_QOS0) {
        httpd_resp_sendstr_chunk(req, "<option selected value=\"ATL_MQTT_QOS0\">At most once (QoS 0)</option> \
                                       <option value=\"ATL_MQTT_QOS1\">At least once (QoS 1)</option> \
                                       <option value=\"ATL_MQTT_QOS2\">Exactly once (QoS 2)</option> \
                                       </select></td></tr>");
    } else if (mqtt_client_config.qos == ATL_MQTT_QOS1) {
        httpd_resp_sendstr_chunk(req, "<option value=\"ATL_MQTT_QOS0\">At most once (QoS 0)</option> \
                                       <option selected value=\"ATL_MQTT_QOS1\">At least once (QoS 1)</option> \
                                       <option value=\"ATL_MQTT_QOS2\">Exactly once (QoS 2)</option> \
                                       </select></td></tr>");
    } else if (mqtt_client_config.qos == ATL_MQTT_QOS2) {
        httpd_resp_sendstr_chunk(req, "<option value=\"ATL_MQTT_QOS0\">At most once (QoS 0)</option> \
                                       <option value=\"ATL_MQTT_QOS1\">At least once (QoS 1)</option> \
                                       <option selected value=\"ATL_MQTT_QOS2\">Exactly once (QoS 2)</option> \
                                       </select></td></tr>");
    }
    httpd_resp_sendstr_chunk(req, "</table><br><div class=\"reboot-msg\" id=\"delayMsg\"></div>");    

    /* Send button chunks */    
    httpd_resp_sendstr_chunk(req, "<br><input class=\"btn_generic\" name=\"btn_save_reboot\" type=\"submit\" \
                                    onclick=\"delayRedirect()\" value=\"Save & Reboot\"></div></form>");     

    /* Send footer chunck */
    const size_t footer_size = (footer_end - footer_start);
    httpd_resp_send_chunk(req, (const char *)footer_start, footer_size);

    /* Send empty chunk to signal HTTP response completion */
    httpd_resp_sendstr_chunk(req, NULL);

    return ESP_OK;
}

/**
 * @brief HTTP GET Handler for MQTT client webpage
 */
static const httpd_uri_t conf_mqtt_get = {
    .uri = "/conf_mqtt.html",
    .method = HTTP_GET,
    .handler = conf_mqtt_get_handler
};

/**
 * @fn conf_mqtt_update_handler(httpd_req_t *req)
 * @brief POST handler for MQTT Client configuration webpage
 * @details HTTP POST handler for MQTT Client configuration webpage
 * @param[in] req - request
 * @return ESP error code
*/
static esp_err_t conf_mqtt_post_handler(httpd_req_t *req) {
    ESP_LOGD(TAG, "Processing POST conf_mqtt_post");

    /* Allocate memory to process request */
    int    ret;
    size_t off = 0;
    char*  buf = calloc(1, req->content_len + 1);
    if (!buf) {
        ESP_LOGE(TAG, "Failed to allocate memory of %d bytes!", req->content_len + 1);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    /* Receive all data */
    while (off < req->content_len) {
        /* Read data received in the request */
        ret = httpd_req_recv(req, buf + off, req->content_len - off);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            }
            free(buf);
            return ESP_FAIL;
        }
        off += ret;
    }
    buf[off] = '\0';

    /* Make a local copy of MQTT client configuration */
    atl_mqtt_client_t mqtt_client_config;
    memset(&mqtt_client_config, 0, sizeof(atl_mqtt_client_t));
    if (xSemaphoreTake(atl_config_mutex, portMAX_DELAY) == pdTRUE) {
        memcpy(&mqtt_client_config, &atl_config.mqtt_client, sizeof(atl_mqtt_client_t));
        xSemaphoreGive(atl_config_mutex);
    }
    else {
        ESP_LOGW(TAG, "Fail to get configuration mutex!");
    }

    /* Search for custom header field */
    char* token;
    char* key;
    char* value;
    int token_len, value_len;
    token = strtok(buf, "&");
    while (token) {
        token_len = strlen(token);
        value = strstr(token, "=") + 1;
        value_len = strlen(value);
        key = calloc(1, (token_len - value_len));
        if (!key) {
            ESP_LOGE(TAG, "Failed to allocate memory!");
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        memcpy(key, token, (token_len - value_len - 1));
        if (strcmp(key, "mqtt_mode") == 0) {
            if (strcmp(value, "ATL_MQTT_DISABLED") == 0) {
                mqtt_client_config.mode = ATL_MQTT_DISABLED;
            } else if (strcmp(value, "ATL_MQTT_AGROTECHLAB_CLOUD") == 0) {
                mqtt_client_config.mode = ATL_MQTT_AGROTECHLAB_CLOUD;
            } else if (strcmp(value, "ATL_MQTT_THIRD") == 0) {
                mqtt_client_config.mode = ATL_MQTT_THIRD;
            }            
            ESP_LOGI(TAG, "Updating [%s:%s]", key,  value);
        } else if (strcmp(key, "mqtt_srv_addr") == 0) {
            strncpy((char*)&mqtt_client_config.broker_address, value, sizeof(mqtt_client_config.broker_address));
            ESP_LOGI(TAG, "Updating [%s:%s]", key,  value);
        } else if (strcmp(key, "mqtt_srv_port") == 0) {
            mqtt_client_config.broker_port = atoi(value);
            ESP_LOGI(TAG, "Updating [%s:%s]", key,  value);
        } else if (strcmp(key, "mqtt_transport") == 0) {
            if (strcmp(value, "MQTT_TRANSPORT_OVER_TCP") == 0) {
                mqtt_client_config.transport = MQTT_TRANSPORT_OVER_TCP;
            } else if (strcmp(value, "MQTT_TRANSPORT_OVER_SSL") == 0) {
                mqtt_client_config.transport = MQTT_TRANSPORT_OVER_SSL;
            }            
            ESP_LOGI(TAG, "Updating [%s:%s]", key,  value);
        } else if (strcmp(key, "mqtt_disable_cn_check") == 0) {
            if (strcmp(value, "true") == 0) {
                mqtt_client_config.disable_cn_check = true;
            } else if (strcmp(value, "false") == 0) {
                mqtt_client_config.disable_cn_check = false;
            }
            ESP_LOGI(TAG, "Updating [%s:%s]", key,  value);
        } else if (strcmp(key, "mqtt_username") == 0) {
            strncpy((char*)&mqtt_client_config.user, value, sizeof(mqtt_client_config.user));
            ESP_LOGI(TAG, "Updating [%s:%s]", key,  value);
        } else if (strcmp(key, "mqtt_pass") == 0) {
            strncpy((char*)&mqtt_client_config.pass, value, sizeof(mqtt_client_config.pass));
            ESP_LOGI(TAG, "Updating [%s:%s]", key,  value);
        } else if (strcmp(key, "mqtt_qos") == 0) {
            if (strcmp(value, "ATL_MQTT_QOS0") == 0) {
                mqtt_client_config.qos = ATL_MQTT_QOS0;
            } else if (strcmp(value, "ATL_MQTT_QOS1") == 0) {
                mqtt_client_config.qos = ATL_MQTT_QOS1;
            } else if (strcmp(value, "ATL_MQTT_QOS2") == 0) {
                mqtt_client_config.qos = ATL_MQTT_QOS2;
            }
            ESP_LOGI(TAG, "Updating [%s:%s]", key,  value);
        }
        free(key);
        token = strtok(NULL, "&");
    }
    
    /* Update current MQTT client configuration */        
    if (xSemaphoreTake(atl_config_mutex, portMAX_DELAY) == pdTRUE) {
        memcpy(&atl_config.mqtt_client, &mqtt_client_config, sizeof(atl_mqtt_client_t));
        xSemaphoreGive(atl_config_mutex);
    }
    else {
        ESP_LOGW(TAG, "Fail to get configuration mutex!");
    }

    /* Commit configuration to NVS */
    atl_config_commit_nvs();    

    /* Restart X200 device */
    ESP_LOGW(TAG, ">>> Rebooting GreenField!");
    atl_led_builtin_blink(10, 100, 255, 69, 0);
    esp_restart();
    return ESP_OK;
}

/**
 * @brief HTTP POST Handler for MQTT client webpage
 */
static const httpd_uri_t conf_mqtt_post = {
    .uri = "/conf_mqtt_post.html",
    .method = HTTP_POST,
    .handler = conf_mqtt_post_handler
};

/**
 * @brief HTTP GET Handler for wifi webpage
 */
static const httpd_uri_t conf_wifi_get = {
    .uri = "/conf_wifi.html",
    .method = HTTP_GET,
    .handler = conf_wifi_get_handler
};

/**
 * @fn conf_wifi_post_handler(httpd_req_t *req)
 * @brief POST handler for WiFi configuration webpage
 * @details HTTP POST handler for WiFi configuration webpage
 * @param[in] req - request
 * @return ESP error code
 */
static esp_err_t conf_wifi_post_handler(httpd_req_t *req) {
    ESP_LOGD(TAG, "Processing POST conf_wifi_post");

    /* Allocate memory to process request */
    int    ret;
    size_t off = 0;
    char*  buf = calloc(1, req->content_len + 1);
    if (!buf) {
        ESP_LOGE(TAG, "Failed to allocate memory of %d bytes!", req->content_len + 1);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    /* Receive all data */
    while (off < req->content_len) {
        /* Read data received in the request */
        ret = httpd_req_recv(req, buf + off, req->content_len - off);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            }
            free(buf);
            return ESP_FAIL;
        }
        off += ret;
    }
    buf[off] = '\0';

    /* Make a local copy of WIFI configuration */
    atl_config_wifi_t wifi_config;
    memset(&wifi_config, 0, sizeof(atl_config_wifi_t));
    if (xSemaphoreTake(atl_config_mutex, portMAX_DELAY) == pdTRUE) {
        memcpy(&wifi_config, &atl_config.wifi, sizeof(atl_config_wifi_t));
        xSemaphoreGive(atl_config_mutex);
    }
    else {
        ESP_LOGW(TAG, "Fail to get configuration mutex!");
    }

    /* Search for custom header field */
    char* token;
    char* key;
    char* value;
    int token_len, value_len;
    token = strtok(buf, "&");
    while (token) {
        token_len = strlen(token);
        value = strstr(token, "=") + 1;
        value_len = strlen(value);
        key = calloc(1, (token_len - value_len));
        if (!key) {
            ESP_LOGE(TAG, "Failed to allocate memory!");
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        memcpy(key, token, (token_len - value_len - 1));
        if (strcmp(key, "wifi_mode") == 0) {
            if (strcmp(value, "AP_MODE") == 0) {
                wifi_config.mode = ATL_WIFI_AP_MODE;
            } else if (strcmp(value, "STA_MODE") == 0) {
                wifi_config.mode = ATL_WIFI_STA_MODE;
            } else if (strcmp(value, "WIFI_DISABLED") == 0) {
                wifi_config.mode = ATL_WIFI_DISABLED;
            }
            ESP_LOGI(TAG, "Updating [%s:%s]", key,  value);
        } else if (strcmp(key, "bssid") == 0) {
            strncpy((char*)&wifi_config.sta_ssid, value, sizeof(wifi_config.sta_ssid));
            ESP_LOGI(TAG, "Updating [%s:%s]", key,  value);
        } else if (strcmp(key, "pass") == 0) {
            strncpy((char*)&wifi_config.sta_pass, value, sizeof(wifi_config.sta_pass));
            ESP_LOGI(TAG, "Updating [%s:%s]", key,  value);
        }
        free(key);
        token = strtok(NULL, "&");
    }
    
    /* Update current WIFI configuration */        
    if (xSemaphoreTake(atl_config_mutex, portMAX_DELAY) == pdTRUE) {
        memcpy(&atl_config.wifi, &wifi_config, sizeof(atl_config_wifi_t));
        xSemaphoreGive(atl_config_mutex);
    }
    else {
        ESP_LOGW(TAG, "Fail to get configuration mutex!");
    }

    /* Commit configuration to NVS */
    atl_config_commit_nvs();    

    /* Restart X200 device */
    ESP_LOGW(TAG, ">>> Rebooting X200!");
    atl_led_builtin_blink(10, 100, 255, 69, 0);
    esp_restart();
    return ESP_OK;
}

/**
 * @brief HTTP POST Handler for wifi webpage
 */
static const httpd_uri_t conf_wifi_post = {
    .uri = "/conf_wifi_post.html",
    .method = HTTP_POST,
    .handler = conf_wifi_post_handler
};

/**
 * @fn conf_configuration_get_handler(httpd_req_t *req)
 * @brief GET handler
 * @details HTTP GET Handler
 * @param[in] req - request
 * @return ESP error code
 */
static esp_err_t conf_configuration_get_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Sending conf_cofiguration.html");    

    /* Set response status, type and header */
    httpd_resp_set_status(req, HTTPD_200);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Connection", "keep-alive");

    /* Send header chunck */
    const size_t header_size = (header_end - header_start);
    httpd_resp_send_chunk(req, (const char *)header_start, header_size);    

    /* Send parameters chunks */            
    httpd_resp_sendstr_chunk(req, "<div class=\"row\"><br><input class=\"btn_generic\" name=\"btn_get_conf\" \
                        onclick=\"getConfJSONFile()\" value=\"Get JSON configuration file\"></div>");
    
    /* Send footer chunck */
    const size_t footer_size = (footer_end - footer_start);
    httpd_resp_send_chunk(req, (const char *)footer_start, footer_size);

    /* Send empty chunk to signal HTTP response completion */
    httpd_resp_sendstr_chunk(req, NULL);

    return ESP_OK;
}

/**
 * @brief HTTP GET Handler for configuration webpage
 */
static const httpd_uri_t conf_configuration_get = {
    .uri = "/conf_configuration.html",
    .method = HTTP_GET,
    .handler = conf_configuration_get_handler
};

/**
 * @fn api_v1_system_conf_handler(httpd_req_t *req)
 * @brief GET handler
 * @details HTTP GET Handler
 * @param[in] req - request
 * @return ESP error code
*/
static esp_err_t api_v1_system_conf_handler(httpd_req_t *req) {    
    ESP_LOGI(TAG, "Processing /api/v1/system/conf");    
    
    /* Make a local copy of GreenField device configuration */
    atl_config_t atl_config_local;
    memset(&atl_config_local, 0, sizeof(atl_config_t));
    if (xSemaphoreTake(atl_config_mutex, portMAX_DELAY) == pdTRUE) {        
        memcpy(&atl_config_local, &atl_config, sizeof(atl_config_t));
        
        /* Set response status, type and header */
        httpd_resp_set_status(req, HTTPD_200);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Connection", "keep-alive");

        /* Release configuration mutext */
        xSemaphoreGive(atl_config_mutex);

        /* Create root JSON object */
        cJSON *root = cJSON_CreateObject();
        esp_app_desc_t app_info;
        const esp_partition_t *partition_info_ptr;
        partition_info_ptr = esp_ota_get_running_partition();
        esp_ota_get_partition_description(partition_info_ptr, &app_info);
        cJSON_AddStringToObject(root, "current_fw_title", app_info.project_name);
        cJSON_AddStringToObject(root, "current_fw_version", app_info.version);
        
        /* Create system JSON object */
        cJSON *root_system = cJSON_CreateObject();
        cJSON_AddStringToObject(root_system, "led_behaviour", atl_led_get_behaviour_str(atl_config_local.system.led_behaviour));
        cJSON_AddItemToObject(root, "system", root_system);

        /* Create ota JSON object */
        cJSON *root_ota = cJSON_CreateObject();
        cJSON_AddStringToObject(root_ota, "behaviour", atl_ota_get_behaviour_str(atl_config_local.ota.behaviour));
        cJSON_AddItemToObject(root, "ota", root_ota);
        
        /* Create wifi JSON object */
        cJSON *root_wifi = cJSON_CreateObject();
        cJSON_AddStringToObject(root_wifi, "mode", atl_wifi_get_mode_str(atl_config_local.wifi.mode));
        cJSON_AddStringToObject(root_wifi, "ap_ssid", (const char*)&atl_config_local.wifi.ap_ssid);
        cJSON_AddStringToObject(root_wifi, "ap_pass", (const char*)&atl_config_local.wifi.ap_pass);
        cJSON_AddNumberToObject(root_wifi, "ap_channel", atl_config_local.wifi.ap_channel);
        cJSON_AddNumberToObject(root_wifi, "ap_max_conn", atl_config_local.wifi.ap_max_conn);
        cJSON_AddStringToObject(root_wifi, "sta_ssid", (const char*)&atl_config_local.wifi.sta_ssid);        
        cJSON_AddStringToObject(root_wifi, "sta_pass", (const char*)&atl_config_local.wifi.sta_pass);
        cJSON_AddNumberToObject(root_wifi, "sta_channel", atl_config_local.wifi.sta_channel);
        cJSON_AddNumberToObject(root_wifi, "sta_max_conn_retry", atl_config_local.wifi.sta_max_conn_retry);
        cJSON_AddItemToObject(root, "wifi", root_wifi);

        /* Create webserver JSON object */
        cJSON *root_webserver = cJSON_CreateObject();        
        cJSON_AddStringToObject(root_webserver, "username", (const char*)&atl_config_local.webserver.username);
        cJSON_AddStringToObject(root_webserver, "password", (const char*)&atl_config_local.webserver.password);        
        cJSON_AddItemToObject(root, "webserver", root_webserver);

        /* Create webserver JSON object */
        cJSON *root_mqtt_client = cJSON_CreateObject();
        cJSON_AddStringToObject(root_ota, "mode", atl_mqtt_get_mode_str(atl_config_local.mqtt_client.mode));
        cJSON_AddStringToObject(root_mqtt_client, "broker_address", (const char*)&atl_config_local.mqtt_client.broker_address);
        cJSON_AddNumberToObject(root_mqtt_client, "broker_port", atl_config_local.mqtt_client.broker_port);        
        cJSON_AddStringToObject(root_mqtt_client, "transport", atl_mqtt_get_transport_str(atl_config_local.mqtt_client.transport));        
        cJSON_AddBoolToObject(root_mqtt_client, "disable_cn_check", atl_config_local.mqtt_client.disable_cn_check);
        cJSON_AddStringToObject(root_mqtt_client, "user", (const char*)&atl_config_local.mqtt_client.user);
        cJSON_AddStringToObject(root_mqtt_client, "pass", (const char*)&atl_config_local.mqtt_client.pass);
        cJSON_AddNumberToObject(root_mqtt_client, "qos", atl_config_local.mqtt_client.qos);
        cJSON_AddItemToObject(root, "mqtt_client", root_mqtt_client);

        /* Put JSON to response message */
        const char *conf_info = cJSON_Print(root);

        /* Sent response */
        httpd_resp_sendstr(req, conf_info);

        /* Free objects */                
        cJSON_Delete(root);
        free((void *)conf_info);

        return ESP_OK;
    }
    else {
        ESP_LOGE(TAG, "Fail to get configuration mutex!");

        /* Set response status, type and header */
        httpd_resp_set_status(req, HTTPD_500_INTERNAL_SERVER_ERROR);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Connection", "keep-alive");

        /* Create system JSON object */
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "error", "Fail to get configuration mutex!");

        /* Put JSON to response message */
        const char *conf_info = cJSON_Print(root);

        /* Sent response */
        httpd_resp_sendstr(req, conf_info);

        /* Free objects */                
        cJSON_Delete(root);
        free((void *)conf_info);

        return ESP_FAIL;
    }
}

/**
 * @brief HTTP GET API Handler for configuration webpage
 */
static const httpd_uri_t api_v1_system_conf = {
    .uri = "/api/v1/system/conf",
    .method = HTTP_GET,
    .handler = api_v1_system_conf_handler
};

/**
 * @fn conf_fw_get_update_handler(httpd_req_t *req)
 * @brief GET handler
 * @details HTTP GET Handler
 * @param[in] req - request
 * @return ESP error code
 */
static esp_err_t conf_fw_update_get_handler(httpd_req_t *req) {
    ESP_LOGD(TAG, "Sending conf_fw_update.html");
    char resp_val[65];

    /* Set response status, type and header */
    httpd_resp_set_status(req, HTTPD_200);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Connection", "keep-alive");

    /* Send header chunck */
    const size_t header_size = (header_end - header_start);
    httpd_resp_send_chunk(req, (const char *)header_start, header_size);

    /* Send information chunks */
    httpd_resp_sendstr_chunk(req, "<table><tr><th>Parameter</th><th>Value</th></tr><tr><td>Firmware version</td><td>");
    esp_app_desc_t app_info;
    const esp_partition_t *partition_info_ptr;
    partition_info_ptr = esp_ota_get_running_partition();
    if (esp_ota_get_partition_description(partition_info_ptr, &app_info) == ESP_OK) {
        sprintf(resp_val, "%s", app_info.version);
        httpd_resp_sendstr_chunk(req, resp_val);
        httpd_resp_sendstr_chunk(req, "</td></tr><tr><td>Build</td><td>");
        sprintf(resp_val, "%s %s", app_info.date, app_info.time);
        httpd_resp_sendstr_chunk(req, resp_val);
        httpd_resp_sendstr_chunk(req, "</td></tr><tr><td>SDK version</td><td>");
        sprintf(resp_val, "%s", app_info.idf_ver);
        httpd_resp_sendstr_chunk(req, resp_val);
        httpd_resp_sendstr_chunk(req, "</td></tr><tr><td>Running partition name</td><td>");
        sprintf(resp_val, "%s", partition_info_ptr->label);
        httpd_resp_sendstr_chunk(req, resp_val);
        httpd_resp_sendstr_chunk(req, "</td></tr><tr><td>Running partition size</td><td>");
        sprintf(resp_val, "%ld bytes", partition_info_ptr->size);
        httpd_resp_sendstr_chunk(req, resp_val);
    }
    const esp_partition_pos_t running_pos  = {
        .offset = partition_info_ptr->address,
        .size = partition_info_ptr->size,
    };
    esp_image_metadata_t data;
    data.start_addr = running_pos.offset;
    esp_image_verify(ESP_IMAGE_VERIFY, &running_pos, &data);
    httpd_resp_sendstr_chunk(req, "</td></tr><tr><td>Running firmware size</td><td>");
    sprintf(resp_val, "%ld bytes", data.image_len);
    httpd_resp_sendstr_chunk(req, resp_val);    
    httpd_resp_sendstr_chunk(req, "</td></tr></table><br><br>");

    /* Make a local copy of WIFI configuration */
    atl_config_ota_t ota_config;
    memset(&ota_config, 0, sizeof(atl_config_ota_t));
    if (xSemaphoreTake(atl_config_mutex, portMAX_DELAY) == pdTRUE) {
        memcpy(&ota_config, &atl_config.ota, sizeof(atl_config_ota_t));
        xSemaphoreGive(atl_config_mutex);
    }
    else {
        ESP_LOGW(TAG, "Fail to get configuration mutex!");
    }

    /* Send parameters chunks */
    httpd_resp_sendstr_chunk(req, "<form action=\"conf_fw_update_post.html\" method=\"post\"> \
                                      <div class=\"row\"> \
                                      <table><tr><th>Parameter</th><th>Value</th></tr> \
                                      <tr><td>FW Update Behaviour</td>");
    
    httpd_resp_sendstr_chunk(req,"<td><select name=\"ota_behaviour\" id=\"ota_behaviour\">");
    if (ota_config.behaviour == ATL_OTA_BEHAVIOUR_DISABLED) {
        httpd_resp_sendstr_chunk(req, "<option selected value=\"ATL_OTA_BEHAVIOUR_DISABLED\">Disabled</option> \
                                       <option value=\"ATL_OTA_BEHAVIOUR_VERIFY_NOTIFY\">Verify & Notify</option> \
                                       <option value=\"ATL_OTA_BEHAVIOU_DOWNLOAD\">Download</option> \
                                       <option value=\"ATL_OTA_BEHAVIOU_DOWNLOAD_REBOOT\">Download & Reboot</option> \
                                       </select></td></tr>");
    } else if (ota_config.behaviour == ATL_OTA_BEHAVIOUR_VERIFY_NOTIFY) {
        httpd_resp_sendstr_chunk(req, "<option value=\"ATL_OTA_BEHAVIOUR_DISABLED\">Disabled</option> \
                                       <option selected value=\"ATL_OTA_BEHAVIOUR_VERIFY_NOTIFY\">Verify & Notify</option> \
                                       <option value=\"ATL_OTA_BEHAVIOU_DOWNLOAD\">Download</option> \
                                       <option value=\"ATL_OTA_BEHAVIOU_DOWNLOAD_REBOOT\">Download & Reboot</option> \
                                       </select></td></tr>");
    } else if (ota_config.behaviour == ATL_OTA_BEHAVIOU_DOWNLOAD) {
        httpd_resp_sendstr_chunk(req, "<option value=\"ATL_OTA_BEHAVIOUR_DISABLED\">Disabled</option> \
                                       <option value=\"ATL_OTA_BEHAVIOUR_VERIFY_NOTIFY\">Verify & Notify</option> \
                                       <option selected value=\"ATL_OTA_BEHAVIOU_DOWNLOAD\">Download</option> \
                                       <option value=\"ATL_OTA_BEHAVIOU_DOWNLOAD_REBOOT\">Download & Reboot</option> \
                                       </select></td></tr>");
    } else if (ota_config.behaviour == ATL_OTA_BEHAVIOU_DOWNLOAD_REBOOT) {
        httpd_resp_sendstr_chunk(req, "<option value=\"ATL_OTA_BEHAVIOUR_DISABLED\">Disabled</option> \
                                       <option value=\"ATL_OTA_BEHAVIOUR_VERIFY_NOTIFY\">Verify & Notify</option> \
                                       <option value=\"ATL_OTA_BEHAVIOU_DOWNLOAD\">Download</option> \
                                       <option selected value=\"ATL_OTA_BEHAVIOU_DOWNLOAD_REBOOT\">Download & Reboot</option> \
                                       </select></td></tr>");
    }
    httpd_resp_sendstr_chunk(req, "</table><br><div class=\"reboot-msg\" id=\"delayMsg\"></div>");

    /* Send button chunks */    
    httpd_resp_sendstr_chunk(req, "<br><input class=\"btn_generic\" name=\"btn_save_reboot\" type=\"submit\" \
                                    onclick=\"delayRedirect()\" value=\"Save & Reboot\"></div></form>"); 
    //httpd_resp_sendstr_chunk(req, "</td></tr></table><br><input class=\"btn_generic\" name=\"btn_upload_fw\" type=\"submit\" value=\"Upload firmware\"></div></form>");

    /* Send footer chunck */
    const size_t footer_size = (footer_end - footer_start);
    httpd_resp_send_chunk(req, (const char *)footer_start, footer_size);

    /* Send empty chunk to signal HTTP response completion */
    httpd_resp_sendstr_chunk(req, NULL);

    return ESP_OK;
}

/**
 * @brief HTTP GET Handler for firmware update webpage
 */
static const httpd_uri_t conf_fw_update_get = {
    .uri = "/conf_fw_update.html",
    .method = HTTP_GET,
    .handler = conf_fw_update_get_handler
};

/**
 * @fn conf_fw_update_post_handler(httpd_req_t *req)
 * @brief POST handler
 * @details HTTP POST Handler
 * @param[in] req - request
 * @return ESP error code
 */
static esp_err_t conf_fw_update_post_handler(httpd_req_t *req) {
    ESP_LOGD(TAG, "Processing POST conf_fw_update_post");

    /* Allocate memory to process request */
    int    ret;
    size_t off = 0;
    char*  buf = calloc(1, req->content_len + 1);
    if (!buf) {
        ESP_LOGE(TAG, "Failed to allocate memory of %d bytes!", req->content_len + 1);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    /* Receive all data */
    while (off < req->content_len) {
        /* Read data received in the request */
        ret = httpd_req_recv(req, buf + off, req->content_len - off);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            }
            free(buf);
            return ESP_FAIL;
        }
        off += ret;
    }
    buf[off] = '\0';

    /* Make a local version of OTA configuration */
    atl_config_ota_t ota_config;
    memset(&ota_config, 0, sizeof(atl_config_ota_t));    

    /* Search for custom header field */
    char* token;
    char* key;
    char* value;
    int token_len, value_len;
    token = strtok(buf, "&");
    while (token) {
        token_len = strlen(token);
        value = strstr(token, "=") + 1;
        value_len = strlen(value);
        key = calloc(1, (token_len - value_len));
        if (!key) {
            ESP_LOGE(TAG, "Failed to allocate memory!");
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        memcpy(key, token, (token_len - value_len - 1));
        if (strcmp(key, "ota_behaviour") == 0) {
            if (strcmp(value, "ATL_OTA_BEHAVIOUR_DISABLED") == 0) {
                ota_config.behaviour = ATL_OTA_BEHAVIOUR_DISABLED;
            } else if (strcmp(value, "ATL_OTA_BEHAVIOUR_VERIFY_NOTIFY") == 0) {
                ota_config.behaviour = ATL_OTA_BEHAVIOUR_VERIFY_NOTIFY;
            } else if (strcmp(value, "ATL_OTA_BEHAVIOU_DOWNLOAD") == 0) {
                ota_config.behaviour = ATL_OTA_BEHAVIOU_DOWNLOAD;
            } else if (strcmp(value, "ATL_OTA_BEHAVIOU_DOWNLOAD_REBOOT") == 0) {
                ota_config.behaviour = ATL_OTA_BEHAVIOU_DOWNLOAD_REBOOT;
            }
            ESP_LOGI(TAG, "Updating [%s:%s]", key,  value);
        } 
        free(key);
        token = strtok(NULL, "&");
    }
    
    if (xSemaphoreTake(atl_config_mutex, portMAX_DELAY) == pdTRUE) {
        memcpy(&ota_config, &atl_config.ota, sizeof(atl_config_ota_t));
        xSemaphoreGive(atl_config_mutex);
    }
    else {
        ESP_LOGW(TAG, "Fail to get configuration mutex!");
    }

    /* Commit configuration to NVS */
    atl_config_commit_nvs();
    
    /* Restart GreenField device */
    ESP_LOGW(TAG, ">>> Rebooting GreenField!");
    atl_led_builtin_blink(10, 100, 255, 69, 0);
    esp_restart();
    return ESP_OK;
}

/**
 * @brief HTTP POST Handler for firmware update webpage
 */
static const httpd_uri_t conf_fw_update_post = {
    .uri = "/conf_fw_update_post.html",
    .method = HTTP_POST,
    .handler = conf_fw_update_post_handler
};

/**
 * @fn conf_reboot_get_handler(httpd_req_t *req)
 * @brief GET handler
 * @details HTTP GET Handler
 * @param[in] req - request
 * @return ESP error code
 */
static esp_err_t conf_reboot_get_handler(httpd_req_t *req) {
    ESP_LOGD(TAG, "Sending conf_reboot.html");
    char resp_val[65];

    /* Set response status, type and header */
    httpd_resp_set_status(req, HTTPD_200);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Connection", "keep-alive");

    /* Send header chunck */
    const size_t header_size = (header_end - header_start);
    httpd_resp_send_chunk(req, (const char *)header_start, header_size);

    /* Send article chunks */
    httpd_resp_sendstr_chunk(req, "<form action=\"conf_reboot_post.html\" method=\"post\"> \
        <div class=\"row\"> \
        <table><tr><th>Parameter</th><th>Value</th></tr> \
        <tr><td>Last reboot reason</td><td>");
    esp_reset_reason_t reset_reason;
    reset_reason = esp_reset_reason();
    if (reset_reason == ESP_RST_UNKNOWN) {
        sprintf(resp_val, "Reset reason can not be determined");
    } else if (reset_reason == ESP_RST_POWERON) {
        sprintf(resp_val, "Reset due to power-on event");
    } else if (reset_reason == ESP_RST_EXT) {
        sprintf(resp_val, "Reset by external pin");
    } else if (reset_reason == ESP_RST_SW) {
        sprintf(resp_val, "Software reset");
    } else if (reset_reason == ESP_RST_PANIC) {
        sprintf(resp_val, "Software reset due to exception/panic");
    } else if (reset_reason == ESP_RST_INT_WDT) {
        sprintf(resp_val, "Reset (software or hardware) due to interrupt watchdog");
    } else if (reset_reason == ESP_RST_TASK_WDT) {
        sprintf(resp_val, "Reset due to task watchdog");
    } else if (reset_reason == ESP_RST_WDT) {
        sprintf(resp_val, "Reset due to other watchdogs");
    } else if (reset_reason == ESP_RST_DEEPSLEEP) {
        sprintf(resp_val, "Reset after exiting deep sleep mode");
    } else if (reset_reason == ESP_RST_BROWNOUT) {
        sprintf(resp_val, "Brownout reset (software or hardware)");
    } else if (reset_reason == ESP_RST_SDIO) {
        sprintf(resp_val, "Reset over SDIO");
    }
    httpd_resp_sendstr_chunk(req, resp_val);
    httpd_resp_sendstr_chunk(req, "</td></tr></table><br><input class=\"btn_generic\" name=\"btn_reboot\" type=\"submit\" value=\"Reboot X200\"></div></form>");

    /* Send footer chunck */
    const size_t footer_size = (footer_end - footer_start);
    httpd_resp_send_chunk(req, (const char *)footer_start, footer_size);

    /* Send empty chunk to signal HTTP response completion */
    httpd_resp_sendstr_chunk(req, NULL);

    return ESP_OK;
}

/**
 * @brief HTTP GET Handler for reboot webpage
 */
static const httpd_uri_t conf_reboot_get = {
    .uri = "/conf_reboot.html",
    .method = HTTP_GET,
    .handler = conf_reboot_get_handler
};

/**
 * @fn conf_reboot_post_handler(httpd_req_t *req)
 * @brief POST handler
 * @details HTTP POST Handler
 * @param[in] req - request
 * @return ESP error code
 */
static esp_err_t conf_reboot_post_handler(httpd_req_t *req) {
    ESP_LOGD(TAG, "Processing POST conf_reboot");

    /* Restart GreenField device */
    ESP_LOGW(TAG, ">>> Rebooting GreenField!");
    atl_led_builtin_blink(10, 100, 255, 69, 0);
    esp_restart();

    return ESP_OK;
}

/**
 * @brief HTTP POST Handler for reboot webpage
 */
static const httpd_uri_t conf_reboot_post = {
    .uri = "/conf_reboot_post.html",
    .method = HTTP_POST,
    .handler = conf_reboot_post_handler
};

/* Basic authentication information */
typedef struct {
    char* username;
    char* password;
} basic_auth_info_t;

/**
 * @fn httpd_auth_basic(const char *username, const char *password)
 * @brief Perform basic authentication
 * @details Perform HTTP basic authentication
 * @param[in] username - username
 * @param[in] password - password
 * @return char* digest
 */
static char *httpd_auth_basic(const char *username, const char *password) {
    int out;
    char *user_info = NULL;
    char *digest = NULL;
    size_t n = 0;
    asprintf(&user_info, "%s:%s", username, password);
    if (!user_info) {
        ESP_LOGE(TAG, "No enough memory for user information");
        return NULL;
    }
    esp_crypto_base64_encode(NULL, 0, &n, (const unsigned char *)user_info, strlen(user_info));
    
    /* 6: The length of the "Basic " string
     * n: Number of bytes for a base64 encode format
     * 1: Number of bytes for a reserved which be used to fill zero
    */
    digest = calloc(1, 6 + n + 1);
    if (digest) {
        strcpy(digest, "Basic ");
        esp_crypto_base64_encode((unsigned char *)digest + 6, n, (size_t *)&out, (const unsigned char *)user_info, strlen(user_info));
    }
    free(user_info);
    return digest;
}

/**
 * @fn basic_auth_get_handler(httpd_req_t *req)
 * @brief GET handler for basic authentication
 * @details HTTP handler for basic authentication
 * @param[in] req - request
 * @return ESP error code
 */
static esp_err_t basic_auth_get_handler(httpd_req_t *req) {
    char *buf = NULL;
    size_t buf_len = 0;
    basic_auth_info_t *basic_auth_info = req->user_ctx;

    buf_len = httpd_req_get_hdr_value_len(req, "Authorization") + 1;
    if (buf_len > 1) {
        buf = calloc(1, buf_len);
        if (!buf) {
            ESP_LOGE(TAG, "No enough memory for basic authorization");
            return ESP_ERR_NO_MEM;
        }

        if (httpd_req_get_hdr_value_str(req, "Authorization", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header -> Authorization: %s", buf);
        } else {
            ESP_LOGE(TAG, "No auth value received!");
        }

        char *auth_credentials = httpd_auth_basic(basic_auth_info->username, basic_auth_info->password);
        if (!auth_credentials) {
            ESP_LOGE(TAG, "No enough memory for basic authorization credentials");
            free(buf);
            return ESP_ERR_NO_MEM;
        }

        if (strncmp(auth_credentials, buf, buf_len)) {
            ESP_LOGE(TAG, "Not authenticated!");
            httpd_resp_set_status(req, HTTPD_401);
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Connection", "keep-alive");
            httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"Hello\"");
            httpd_resp_send(req, NULL, 0);
        } else {
            ESP_LOGI(TAG, "Authenticated!");
            home_get_handler(req);            
        }
        free(auth_credentials);
        free(buf);
    } else {
        ESP_LOGE(TAG, "No auth header received!");
        httpd_resp_set_status(req, HTTPD_401);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Connection", "keep-alive");
        httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"Hello\"");
        httpd_resp_send(req, NULL, 0);
    }

    return ESP_OK;
}

/**
 * @brief HTTP GET Handler for basic authentication
 */
static httpd_uri_t basic_auth = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = basic_auth_get_handler,
};

/**
 * @brief Basic authentication configuration
 */
static void httpd_register_basic_auth(httpd_handle_t server) {
    basic_auth_info_t *basic_auth_info = calloc(1, sizeof(basic_auth_info_t));    
    
    /* Get cofiguration mutex to setup webserver config */    
    if (xSemaphoreTake(atl_config_mutex, portMAX_DELAY) == pdTRUE) {
        basic_auth_info->username = calloc(1, sizeof(atl_config.webserver.username));
        snprintf((char*)basic_auth_info->username, sizeof(atl_config.webserver.username), "%s", atl_config.webserver.username);
        basic_auth_info->password = calloc(1, sizeof(atl_config.webserver.password));
        snprintf((char*)basic_auth_info->password, sizeof(atl_config.webserver.password), "%s", atl_config.webserver.password);        

        /* Release cofiguration mutex */
        xSemaphoreGive(atl_config_mutex);
    }
    else {
        ESP_LOGW(TAG, "Fail to get configuration mutex!");
    }    

    basic_auth.user_ctx = basic_auth_info;
    httpd_register_uri_handler(server, &basic_auth);
}

/**
 * @fn https_server_user_callback(esp_https_server_user_cb_arg_t *user_cb)
 * @brief HTTPS server callback
*/
static void https_server_user_callback(esp_https_server_user_cb_arg_t *user_cb) {
    mbedtls_ssl_context *ssl_ctx = NULL;
    switch(user_cb->user_cb_state) {
        case HTTPD_SSL_USER_CB_SESS_CREATE:
            ESP_LOGI(TAG, "HTTPS session creation");

            /* Logging the socket FD */
            int sockfd = -1;
            esp_err_t esp_ret;
            esp_ret = esp_tls_get_conn_sockfd(user_cb->tls, &sockfd);
            if (esp_ret != ESP_OK) {
                ESP_LOGE(TAG, "Error in obtaining the sockfd from tls context");
                break;
            }
            ESP_LOGI(TAG, "Socket FD: %d", sockfd);
            ssl_ctx = (mbedtls_ssl_context *) esp_tls_get_ssl_context(user_cb->tls);
            if (ssl_ctx == NULL) {
                ESP_LOGE(TAG, "Error in obtaining ssl context");
                break;
            }
            /* Logging the current ciphersuite */
            ESP_LOGI(TAG, "Current Ciphersuite: %s", mbedtls_ssl_get_ciphersuite(ssl_ctx));
            break;

        case HTTPD_SSL_USER_CB_SESS_CLOSE:
            ESP_LOGI(TAG, "HTTPS session close");

            /* Logging the peer certificate */
            ssl_ctx = (mbedtls_ssl_context *) esp_tls_get_ssl_context(user_cb->tls);
            if (ssl_ctx == NULL) {
                ESP_LOGE(TAG, "Error in obtaining ssl context");
                break;
            }
            // print_peer_cert_info(ssl_ctx);
            break;

        default:
            ESP_LOGE(TAG, "HTTPS illegal state!");
            return;
    }
}

/**
 * @fn atl_webserver_init(void)
 * @brief Initialize Webserver.
 * @return httpd_handle_t webserver handle.
 */
httpd_handle_t atl_webserver_init(void) {
    httpd_handle_t server = NULL;

    /* Creates default webserver configuration */
    httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();    
    config.httpd.max_uri_handlers = 25;
    config.httpd.max_open_sockets = 7;
    config.httpd.lru_purge_enable = true;
    config.user_cb = https_server_user_callback;
    config.servercert = servercert_start;
    config.servercert_len = servercert_end - servercert_start;
    config.prvtkey_pem = prvtkey_pem_start;
    config.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

    /* Start the HTTPS server */     
    if (httpd_ssl_start(&server, &config) == ESP_OK) {
        
        /* Set URI handlers */
        ESP_LOGD(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &favicon);
        httpd_register_uri_handler(server, &css);
        httpd_register_uri_handler(server, &js);
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);        
        httpd_register_uri_handler(server, &home_get);        
        httpd_register_uri_handler(server, &conf_mqtt_get);
        httpd_register_uri_handler(server, &conf_mqtt_post);        
        httpd_register_uri_handler(server, &conf_wifi_get);
        httpd_register_uri_handler(server, &conf_wifi_post);
        // httpd_register_uri_handler(server, &conf_ethernet_get);
        // httpd_register_uri_handler(server, &conf_4g_get);
        // httpd_register_uri_handler(server, &conf_lora_get);
        httpd_register_uri_handler(server, &conf_configuration_get);
        httpd_register_uri_handler(server, &api_v1_system_conf);
        // httpd_register_uri_handler(server, &conf_configuration_post);
        httpd_register_uri_handler(server, &conf_fw_update_get);
        httpd_register_uri_handler(server, &conf_fw_update_post);
        httpd_register_uri_handler(server, &conf_reboot_get);
        httpd_register_uri_handler(server, &conf_reboot_post);
        httpd_register_basic_auth(server);               
    } else {        
        ESP_LOGE(TAG, "Fail starting webserver!");
    }
    return server;
}