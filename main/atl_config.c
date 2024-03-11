/**
 * @file atl_config.h
 * @author Robson Costa (robson.costa@ifsc.edu.br)
 * @brief Configuration functions.
 * @version 0.1.0
 * @date 2024-03-10 (created)
 * @date 2024-03-11 (updated)
 * 
 * @copyright Copyright &copy; since 2024 <a href="https://agrotechlab.lages.ifsc.edu.br" target="_blank">AgroTechLab</a>.\n
 * ![LICENSE license](../figs/license.png)<br>
 * Licensed under the CC BY-SA (<i>Creative Commons Attribution-ShareAlike</i>) 4.0 International Unported License (the <em>"License"</em>). You may not
 * use this file except in compliance with the License. You may obtain a copy of the License <a href="https://creativecommons.org/licenses/by-sa/4.0/legalcode" target="_blank">here</a>.
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an <em>"as is" basis, 
 * without warranties or  conditions of any kind</em>, either express or implied. See the License for the specific language governing permissions 
 * and limitations under the License.
 */

#include <string.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <nvs.h>
#include "sdkconfig.h"
#include "atl_config.h"

/* Constants */
static const char *TAG = "atl-config";

/* Global variables */
SemaphoreHandle_t atl_config_mutex;
atl_config_t atl_config;

/**
 * @fn atl_config_create_default(void)
 * @brief Create configuration file with default values.
 * @details Create configuration file with default values. 
 */
static void atl_config_create_default(void) {    
    char ssid[32];
    unsigned char mac[6] = {0};

    /** Creates default SYSTEM configuration **/
    atl_config.system.led_behaviour = ATL_LED_ENABLED_FULL;

    /** Creates default WiFi configuration **/
    atl_config.wifi.mode = ATL_WIFI_AP_MODE;
    esp_efuse_mac_get_default(mac);
    sprintf(ssid, "%s%02x%02x%02x", CONFIG_ATL_WIFI_AP_SSID_PREFIX, mac[3], mac[4], mac[5]+1);
    strncpy((char*)&atl_config.wifi.ap_ssid, ssid, sizeof(atl_config.wifi.ap_ssid));
    strncpy((char*)&atl_config.wifi.ap_pass, CONFIG_ATL_WIFI_AP_PASSWORD, sizeof(atl_config.wifi.ap_pass));
    atl_config.wifi.ap_channel = CONFIG_ATL_WIFI_AP_CHANNEL;
    atl_config.wifi.ap_max_conn = CONFIG_ATL_WIFI_AP_MAX_STA_CONN;
    strncpy((char*)&atl_config.wifi.sta_ssid, "AgroTechLab", sizeof(atl_config.wifi.sta_ssid));
    strncpy((char*)&atl_config.wifi.sta_pass, CONFIG_ATL_WIFI_AP_PASSWORD, sizeof(atl_config.wifi.sta_pass));
    atl_config.wifi.sta_channel = CONFIG_ATL_WIFI_AP_CHANNEL;
    atl_config.wifi.sta_max_conn_retry = CONFIG_ATL_WIFI_STA_MAX_CONN_RETRY;    
}

/**
 * @fn atl_config_init(void)
 * @brief Initialize configuration from NVS.
 * @details If not possible load configuration file, create a new with default values.
 * @return esp_err_t - If ERR_OK success, otherwise fail.
 */
esp_err_t atl_config_init(void) {
    esp_err_t err;
    nvs_handle_t nvs_handler;
    
    /* Creates configuration semaphore (mutex) */
    atl_config_mutex = xSemaphoreCreateMutex();
    if (atl_config_mutex == NULL) {
        ESP_LOGE(TAG, "Error creating configuration semaphore!");
        return ESP_FAIL;        
    }

    /* Open NVS system */
    ESP_LOGI(TAG, "Loading configuration from NVS");
    ESP_LOGI(TAG, "Mounting NVS storage");
    err = nvs_open("nvs", NVS_READWRITE, &nvs_handler);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fail mounting NVS storage");
        ESP_LOGE(TAG, "Error: %s", esp_err_to_name(err));
        return ESP_FAIL;
    }

    /* Read the memory size required to configuration file */
    ESP_LOGI(TAG, "Loading configuration file");
    size_t file_size = sizeof(atl_config_t);
    err = nvs_get_blob(nvs_handler, "atl_config", &atl_config, &file_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Fail loading configuration file!");
        goto error_proc;
    } else if (err != ESP_OK) {
        ESP_LOGW(TAG, "File not found! Creating new file with default values!");
        atl_config_create_default();

        /* Creates greenfield_config file */
        err = nvs_set_blob(nvs_handler, "atl_config", &atl_config, sizeof(atl_config_t));
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Fail creating new configuration file!");
            goto error_proc;
        }

        /* Write atl_config file in NVS */
        err = nvs_commit(nvs_handler);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Fail writing new configuration file!");
            goto error_proc;
        } 
    }

    /* Close NVS */
    ESP_LOGI(TAG, "Unmounting NVS storage");
    nvs_close(nvs_handler);
    return ESP_OK;

    /* Error procedure */
    error_proc:
        ESP_LOGE(TAG, "Error: %s", esp_err_to_name(err));
        nvs_close(nvs_handler);
        return err;
}
