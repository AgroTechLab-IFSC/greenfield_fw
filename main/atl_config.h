/**
 * @file atl_config.h
 * @author Robson Costa (robson.costa@ifsc.edu.br)
 * @brief Configuration header.
 * @version 0.1.0
 * @date 2024-03-08 (created)
 * @date 2024-03-10 (updated)
 * 
 * @copyright Copyright &copy; since 2024 <a href="https://agrotechlab.lages.ifsc.edu.br" target="_blank">AgroTechLab</a>.\n
 * ![LICENSE license](../figs/license.png)<br>
 * Licensed under the CC BY-SA (<i>Creative Commons Attribution-ShareAlike</i>) 4.0 International Unported License (the <em>"License"</em>). You may not
 * use this file except in compliance with the License. You may obtain a copy of the License <a href="https://creativecommons.org/licenses/by-sa/4.0/legalcode" target="_blank">here</a>.
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an <em>"as is" basis, 
 * without warranties or  conditions of any kind</em>, either express or implied. See the License for the specific language governing permissions 
 * and limitations under the License.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <inttypes.h>
#include <esp_err.h>
#include "atl_led.h"
#include "atl_wifi.h"

/* External global variable */
extern SemaphoreHandle_t atl_config_mutex;

/**
 * @typedef atl_ota_behaviour_e
 * @brief ATL OTA behaviour.
 */
typedef enum {	
    ATL_OTA_BEHAVIOUR_DISABLED,
	ATL_OTA_BEHAVIOUR_VERIFY_NOTIFY,
	ATL_OTA_BEHAVIOU_DOWNLOAD,
	ATL_OTA_BEHAVIOU_DOWNLOAD_REBOOT,    
} atl_ota_behaviour_e;

/**
 * @typedef atl_config_system_t
 * @brief System configuration structure.
 */
typedef struct {
    atl_led_behaviour_e    led_behaviour; /**< LED behaviour. */
} atl_config_system_t;

/**
 * @typedef atl_config_ota_t
 * @brief OTA configuration structure.
 */
typedef struct {
    atl_ota_behaviour_e    behaviour; /**< OTA behaviour. */
} atl_config_ota_t;

/**
 * @brief atl_config_wifi_t
 * @brief WiFi configuration structure.
 */
typedef struct {
    atl_wifi_mode_e mode; /**< WiFi mode. */
    uint8_t ap_ssid[32]; /**< WiFi AP SSID.*/
    uint8_t ap_pass[64]; /**< WiFi AP password.*/
    uint8_t ap_channel; /**< WiFi AP channel.*/
    uint8_t ap_max_conn; /**< WiFi AP maximum STA connections.*/
    uint8_t sta_ssid[32]; /**< WiFi STA SSID.*/
    uint8_t sta_pass[64]; /**< WiFi STA password.*/
    uint8_t sta_channel; /**< WiFi STA channel.*/
    uint8_t sta_max_conn_retry; /**< WiFi maximum connection retry.*/
} atl_config_wifi_t;

/**
 * @brief atl_config_webserver_t
 * @brief Webserver configuration structure.
 */
typedef struct {    
    uint8_t username[32]; /**< WiFi AP SSID.*/
    uint8_t password[64]; /**< WiFi AP password.*/
} atl_config_webserver_t;

/**
 * @typedef atl_config_t
 * @brief Configuration structure.
 */
typedef struct {
    atl_config_system_t     system;     /**< System configuration. */
    atl_config_ota_t        ota;        /**< OTA configuration. */
    atl_config_wifi_t       wifi;       /**< WiFi configuration. */
    atl_config_webserver_t  webserver;  /**< Webserver configuration. */
} atl_config_t;

/**
 * @fn atl_config_init(void)
 * @brief Initialize configuration from NVS.
 * @details If not possible load configuration file, create a new with default values.
 * @return esp_err_t - If ERR_OK success. 
 */
esp_err_t atl_config_init(void);

/**
 * @fn atl_config_commit_nvs(void)
 * @brief Initialize configuration from NVS.
 * @details If not possible load configuration file, create a new with default values.
 * @return esp_err_t - If ERR_OK success, otherwise fail.
 */
esp_err_t atl_config_commit_nvs(void);

#ifdef __cplusplus
}
#endif