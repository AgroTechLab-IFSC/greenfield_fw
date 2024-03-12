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
#include <esp_log.h>
#include <esp_err.h>
#include <esp_https_server.h>
#include <esp_tls_crypto.h>
#include "atl_webserver.h"
#include "atl_config.h"

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
        ESP_LOGW(TAG, "User: %s", basic_auth_info->username);
        ESP_LOGW(TAG, "Pass: %s", basic_auth_info->password);

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
        // httpd_register_uri_handler(server, &conf_moisture_get);
        // httpd_register_uri_handler(server, &conf_moisture_post);
        // httpd_register_uri_handler(server, &conf_analog_input_get);
        // httpd_register_uri_handler(server, &conf_analog_input_post);
        // httpd_register_uri_handler(server, &conf_mqtt_get);
        // httpd_register_uri_handler(server, &conf_mqtt_post);        
        // httpd_register_uri_handler(server, &conf_wifi_get);
        // httpd_register_uri_handler(server, &conf_wifi_post);
        // httpd_register_uri_handler(server, &conf_ethernet_get);
        // httpd_register_uri_handler(server, &conf_4g_get);
        // httpd_register_uri_handler(server, &conf_lora_get);
        // httpd_register_uri_handler(server, &conf_configuration_get);
        // httpd_register_uri_handler(server, &api_v1_system_conf);
        // httpd_register_uri_handler(server, &conf_configuration_post);
        // httpd_register_uri_handler(server, &conf_fw_update_get);
        // httpd_register_uri_handler(server, &conf_fw_update_post);
        // httpd_register_uri_handler(server, &conf_reboot_get);
        // httpd_register_uri_handler(server, &conf_reboot_post);
        httpd_register_basic_auth(server);               
    } else {        
        ESP_LOGE(TAG, "Fail starting webserver!");
    }
    return server;
}