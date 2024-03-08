/**
 * @file atl_storage.h
 * @author Robson Costa (robson.costa@ifsc.edu.br)
 * @brief Storage header.
 * @version 0.1.0
 * @date 2024-03-08 (created)
 * @date 2024-03-08 (updated)
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

/**
 * @fn atl_storage_init(void)
 * @brief Initialize NVS (Non-Volatile Storage).
 * @details Initialize NVS system. If errors ESP_ERR_NVS_NO_FREE_PAGES or ESP_ERR_NVS_NEW_VERSION_FOUND occurs, erase flash and retry initialization.
 * @return esp_err_t - If ERR_OK success. 
 */
esp_err_t atl_storage_init(void);

/**
 * @fn atl_storage_erase_nvs(void)
 * @brief Erase NVS (Non-Volatile Storage).
 * @details Erase NVS.
 * @return esp_err_t - If ERR_OK success. 
 */
esp_err_t atl_storage_erase_nvs(void);

#ifdef __cplusplus
}
#endif