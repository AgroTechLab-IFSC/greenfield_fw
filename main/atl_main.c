/**
 * @file main.c
 * @author Robson Costa (robson.costa@ifsc.edu.br)
 * @brief Main function file.
 * @version 0.1.0
 * @date 2024-02-26 (created)
 * @date 2024-03-10 (updated)
 * 
 * @copyright Copyright &copy; since 2024 <a href="https://agrotechlab.lages.ifsc.edu.br" target="_blank">AgroTechLab</a>.\n
 * ![LICENSE license](../figs/license.png)<br>
 * Licensed under the CC BY-SA (<i>Creative Commons Attribution-ShareAlike</i>) 4.0 International Unported License (the <em>"License"</em>). You may not
 * use this file except in compliance with the License. You may obtain a copy of the License <a href="https://creativecommons.org/licenses/by-sa/4.0/legalcode" target="_blank">here</a>.
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an <em>"as is" basis, 
 * without warranties or conditions of any kind</em>, either express or implied. See the License for the specific language governing permissions 
 * and limitations under the License.
 */
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include "sdkconfig.h"
#include "atl_led.h"
#include "atl_button.h"
#include "atl_storage.h"
#include "atl_config.h"
#include "atl_wifi.h"
#include "atl_dns.h"
#include "atl_webserver.h"

/* Constants */
static const char *TAG = "atl-main";

/* Global external variable */
extern atl_config_t atl_config;

/**
 * @brief Application main function.
 */
void app_main(void) {    

    /* LED builtin initialization */
    atl_led_builtin_init();

    /* Button initialization */
    atl_button_init();

    /* Storage initialization */
    atl_storage_init();
    
    /* Cofiguration initialization (load configuration from NVS or create new default config) */
    atl_config_init();

    /* Check the WiFi startup mode defined by configuration file */
    if (atl_config.wifi.mode != ATL_WIFI_DISABLED) {
        
        /* If in Access Point mode */
        if (atl_config.wifi.mode == ATL_WIFI_AP_MODE) {
            
            /* Initialize WiFi in AP mode */
            atl_wifi_init_softap();

            /* Initialize name server (DNS) */
            // atl_dns_server_init();

        } else if (atl_config.wifi.mode == ATL_WIFI_STA_MODE) {
            
            /* Initialize WiFi in STA mode */
            atl_wifi_init_sta();

        }

        /* Initialize webserver (HTTPS) */
        atl_webserver_init();
    }

    /* Update serial interface output */
    ESP_LOGI(TAG, "Initialization finished!");
}
