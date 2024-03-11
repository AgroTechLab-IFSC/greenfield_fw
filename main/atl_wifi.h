/**
 * @file atl_wifi.h
 * @author Robson Costa (robson.costa@ifsc.edu.br)
 * @brief Wifi header.
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
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

/**
 * @enum    atl_wifi_mode_e
 * @brief   WiFi mode.
 */
typedef enum {
    ATL_WIFI_DISABLED,
    ATL_WIFI_AP_MODE,
    ATL_WIFI_STA_MODE,   
} atl_wifi_mode_e;

/**
 * @fn atl_wifi_init_softap(void)
 * @brief Initialize WiFi interface in SoftAP mode.
 * @return esp_err_t - If ERR_OK success, otherwise fail.
 */
esp_err_t atl_wifi_init_softap(void);

/**
 * @fn atl_wifi_init_sta(void)
 * @brief Initialize WiFi interface in STA mode.
 * @return esp_err_t - If ERR_OK success, otherwise fail.
 */
esp_err_t atl_wifi_init_sta(void);

#ifdef __cplusplus
}
#endif