/**
 * @file atl_button.c
 * @author Robson Costa (robson.costa@ifsc.edu.br)
 * @brief Button functions.
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
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include "atl_led.h"

/* Constants */
static const char *TAG = "atl-button"; /**< Function identification */

/* Global variables */
static QueueHandle_t button_evt_queue; /**< Button event queue */
bool button_pressed = false; /**< Button pressed */

/**
 * @fn button_isr_handler(void *args)
 * @brief Button event handler
 * @param [in] args - Pointer to task arguments
*/
static void IRAM_ATTR button_isr_handler(void *args) {
    uint32_t button_pin = (uint32_t)args;
    xQueueSendFromISR(button_evt_queue, &button_pin, NULL);  
}

/**
 * @fn atl_button_task(void *args)
 * @brief Button task
 * @param [in] args - Pointer to task arguments 
*/
static void atl_button_task(void *args) {
    uint32_t gpio_pin;

    /* Task looping */
    while (true) {

        /* Check for button event */
        if (xQueueReceive(button_evt_queue, &gpio_pin, portMAX_DELAY)) {
            if (gpio_get_level(CONFIG_ATL_BUTTON_GPIO) == 0) {
                button_pressed = true;
                atl_led_set_color(255, 69, 0);                
            } else {
                button_pressed = false;
                atl_led_set_color(0, 0, 255);
            }             
        }
    }    
}

/**
 * @fn atl_button_init(void)
 * @brief Initialize button task
*/
void atl_button_init(void) {
    ESP_LOGI(TAG, "Creating button task at CPU 1");

    /* Configure button event */
    gpio_set_direction(CONFIG_ATL_BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_pulldown_en(CONFIG_ATL_BUTTON_GPIO);
    gpio_pullup_dis(CONFIG_ATL_BUTTON_GPIO);
    gpio_set_intr_type(CONFIG_ATL_BUTTON_GPIO, GPIO_INTR_ANYEDGE);
    button_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    /* Create LED builtin task at CPU 1 */
    xTaskCreatePinnedToCore(atl_button_task, "atl_button_task", 2048, NULL, 10, &atl_button_handle, 1);

    /* Install interruption handler at button event */
    gpio_install_isr_service(0);
    gpio_isr_handler_add(CONFIG_ATL_BUTTON_GPIO, button_isr_handler, (void*)CONFIG_ATL_BUTTON_GPIO);
}