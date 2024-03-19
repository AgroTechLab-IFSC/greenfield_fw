/**
 * @file atl_wifi.c
 * @author Robson Costa (robson.costa@ifsc.edu.br)
 * @brief Wifi function.
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
#include <string.h>
// #include <freertos/event_groups.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_mac.h>
#include "atl_config.h"
// #include "atl_led.h"
#include "atl_wifi.h"

/* Constants */
static const char *TAG = "atl-wifi";
const char *atl_wifi_mode_str[] = {
    "ATL_WIFI_DISABLED",
    "ATL_WIFI_AP_MODE",
    "ATL_WIFI_STA_MODE",
};

/* Global variables */
static EventGroupHandle_t s_wifi_event_group;   /* FreeRTOS event group to signal when we are connected */

/* Global external variables */
extern atl_config_t atl_config;

/**
 * @brief Get the wifi mode enum
 * @param mode_str 
 * @return Function enum
 */
atl_wifi_mode_e atl_wifi_get_mode(char* mode_str) {
    uint8_t i = 0;
    while (atl_wifi_mode_str[i] != NULL) {
        if (strcmp(mode_str, atl_wifi_mode_str[i]) == 0) {
            return i;
        } else {
            i++;
        }
    }
    return 255;
}

/**
 * @fn atl_wifi_event_handler(void* handler_args, esp_event_base_t event_base, int32_t event_id, void* event_data)
 * @brief Event handler registered to receive WiFi events.
 * @details This function is called by the WiFi client event loop.
 * @param[in] handler_args - User data registered to the event.
 * @param[out] event_base - Event base for the handler (always WiFi Base).
 * @param[out] event_id - The id for the received event.
 * @param[out] event_data - The data for the event, esp_mqtt_event_handle_t.
 */
static void atl_wifi_event_handler(void* handler_args, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    uint8_t conn_retry = 0;

    /* Check if WiFi interface was started and then connect to AP */
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } 
    
    /* Check if station was connected */
    else if (event_id == WIFI_EVENT_STA_CONNECTED) {
        wifi_event_sta_connected_t* event = (wifi_event_sta_connected_t*) event_data;
        ESP_LOGI(TAG, "Connected at %s ("MACSTR")", event->ssid, MAC2STR(event->bssid));
    }

    /* Check if station was disconnected */
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*) event_data;
        ESP_LOGI(TAG, "Disconnected from %s ("MACSTR") reason: %d", event->ssid, MAC2STR(event->bssid), event->reason);
        if (conn_retry < atl_config.wifi.sta_max_conn_retry) {
            esp_wifi_connect();
            conn_retry++;
            ESP_LOGW(TAG, "Retry to connect to the AP");
        } else {
            ESP_LOGE(TAG,"Connect to the AP fail");  
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);            
        }
    }

    /* Check if station got IP address */
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));       
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }    

    /* Check if some station connects to AP */
    else if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
    } 

    /* Check if some station disconnects from AP */
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid);
    }
}

/**
 * @brief Get the wifi mode string object
 * @param mode 
 * @return Function enum const* 
 */
const char* atl_wifi_get_mode_str(atl_wifi_mode_e mode) {
    return atl_wifi_mode_str[mode];
}

/**
 * @fn atl_wifi_init_softap(void)
 * @brief Initialize WiFi interface in SoftAP mode.
 * @return esp_err_t - If ERR_OK success, otherwise fail.
 */
esp_err_t atl_wifi_init_softap(void) {
    esp_err_t err = ESP_OK;
    wifi_config_t wifi_config;

    ESP_LOGI(TAG, "Starting GreenField in AP mode!");

    /* Initialize loopback interface */
    err = esp_netif_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fail initializing WiFi loopback interface!");
        goto error_proc;
    }

    /* Initialize event loop */
    err = esp_event_loop_create_default();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fail creating WiFi event loop!");
        goto error_proc;
    }

    /* Initialize default WiFi AP */
    esp_netif_create_default_wifi_ap();

    /* Get default WiFi station configuration */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    /* Initialize WiFi interface with default configuration */
    err = esp_wifi_init(&cfg); 
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fail initializing WiFi with default configuration!");
        goto error_proc;
    }

    /* Register event handlers */ 
    err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &atl_wifi_event_handler, NULL, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fail registering WiFi event handler!");
        goto error_proc;
    }
    
    /* Get cofiguration mutex to setup softAP WiFi config */
    memset(&wifi_config, 0, sizeof(wifi_config_t));
    if (xSemaphoreTake(atl_config_mutex, portMAX_DELAY) == pdTRUE) {
        wifi_config.ap.ssid_len = strlen((const char *)&atl_config.wifi.ap_ssid);
        snprintf((char*)&wifi_config.ap.ssid, sizeof(wifi_config.ap.ssid), "%s", atl_config.wifi.ap_ssid);
        snprintf((char*)&wifi_config.ap.password, sizeof(wifi_config.ap.password), "%s", atl_config.wifi.ap_pass);
        wifi_config.ap.channel = atl_config.wifi.ap_channel;
        wifi_config.ap.max_connection = atl_config.wifi.ap_max_conn;
        wifi_config.ap.authmode = WIFI_AUTH_WPA2_WPA3_PSK;
        wifi_config.ap.pmf_cfg.required = false;
        
        /* Release cofiguration mutex */
        xSemaphoreGive(atl_config_mutex);
    }
    else {
        ESP_LOGE(TAG, "Fail to get configuration mutex!");
        err = ESP_FAIL;
        goto error_proc;
    }
       
    /* If no password was defined, network will be OPEN */
    if (strlen((const char *)wifi_config.ap.password) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    /* Setup WiFi to Access Point Mode */
    err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fail setting WiFi to Access Point mode!");
        goto error_proc;
    }

    /* Apply custom configuration */
    err = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fail applying custom configuration to WiFi interface!");
        goto error_proc;
    }

    /* Start WiFi interface */
    err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fail starting WiFi interface!");
        goto error_proc;
    }
   
    return err;

    /* Error procedure */
error_proc:
    // atl_led_set_color(255, 0, 0);
    ESP_LOGE(TAG, "Error: %d = %s", err, esp_err_to_name(err));        
    return err;
}

/**
 * @fn atl_wifi_init_sta(void)
 * @brief Initialize WiFi interface in STA mode.
 * @return esp_err_t - If ERR_OK success, otherwise fail.
 */
esp_err_t atl_wifi_init_sta(void) {
    esp_err_t err = ESP_OK;
    wifi_config_t wifi_config;

    s_wifi_event_group = xEventGroupCreate();
    ESP_LOGI(TAG, "Starting GreenField in STA mode!");

    /* Initialize loopback interface */
    err = esp_netif_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fail starting WiFi network interface!");
        goto error_proc;
    }

    /* Initialize event loop */
    err = esp_event_loop_create_default();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fail creating WiFi event loop!");
        goto error_proc;
    }

    /* Initialize default WiFi station */
    esp_netif_create_default_wifi_sta();

    /* Get default WiFi station configuration */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    /* Initialize WiFi interface with default configuration */
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fail initializing WiFi with default configuration!");
        goto error_proc;
    }
    
    /* Register event handlers to WiFi */
    esp_event_handler_instance_t instance_any_id;
    err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &atl_wifi_event_handler, NULL, &instance_any_id);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fail registering WiFi event handler!");
        goto error_proc;
    }

    /* Register event handlers to IP */
    esp_event_handler_instance_t instance_got_ip;
    err = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &atl_wifi_event_handler, NULL, &instance_got_ip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fail registering IP event handler!");
        goto error_proc;
    }
    
    /* Get cofiguration mutex to setup station WiFi config */
    memset(&wifi_config, 0, sizeof(wifi_config_t));
    if (xSemaphoreTake(atl_config_mutex, portMAX_DELAY) == pdTRUE) {
        snprintf((char*)&wifi_config.sta.ssid, sizeof(wifi_config.sta.ssid), "%s", atl_config.wifi.sta_ssid);
        snprintf((char*)&wifi_config.sta.password, sizeof(wifi_config.sta.password), "%s", atl_config.wifi.sta_pass);
        wifi_config.sta.channel = atl_config.wifi.sta_channel;
        
        /* Release cofiguration mutex */
        xSemaphoreGive(atl_config_mutex);
    }
    else {
        ESP_LOGE(TAG, "Fail to get configuration mutex!");
        err = ESP_FAIL;
        goto error_proc;
    }    

    /* Setup WiFi to Station Mode */
    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fail setting WiFi to Station mode!");
        goto error_proc;
    }

    /* Apply custom configuration */
    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fail applying custom configuration to WiFi interface!");
        goto error_proc;
    }

    /* Start WiFi interface */
    err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fail starting WiFi interface!");
        goto error_proc;
    }
       
    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
    
    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to SSID %s with password %s",
                 wifi_config.sta.ssid, wifi_config.sta.password);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect SSID %s with password %s",
                 wifi_config.sta.ssid, wifi_config.sta.password);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
    
    return err;
 
    /* Error procedure */
error_proc:
    //atl_led_set_color(255, 0, 0);
    ESP_LOGE(TAG, "Error: %d = %s", err, esp_err_to_name(err));        
    return err;
}