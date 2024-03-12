/**
 * @file atl_config.c
 * @author Robson Costa (robson.costa@ifsc.edu.br)
 * @brief Storage functions.
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
#include <string.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_err.h>
// #include "atl_config.h"
#include "sdkconfig.h"

static const char *TAG = "atl-storage";    /**< Function identification */

/**
 * @fn atl_storage_init(void)
 * @brief Initialize NVS (Non-Volatile Storage).
 * @details Initialize NVS system. If errors ESP_ERR_NVS_NO_FREE_PAGES or ESP_ERR_NVS_NEW_VERSION_FOUND occurs, erase flash and retry initialization.
 * @return esp_err_t - If ERR_OK success. 
 */
esp_err_t atl_storage_init(void) {
    nvs_flash_erase();
    ESP_LOGI(TAG, "Starting NVS (Non-Volatile Storage)");
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing and restarting NVS");
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_flash_erase());
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_flash_init());
    }
    return err;
}

/**
 * @fn atl_storage_erase_nvs(void)
 * @brief Erase NVS (Non-Volatile Storage).
 * @details Erase NVS.
 * @return esp_err_t - If ERR_OK success. 
 */
esp_err_t atl_storage_erase_nvs(void) {
    esp_err_t err = ESP_OK;
    nvs_flash_erase();
    return err;
}