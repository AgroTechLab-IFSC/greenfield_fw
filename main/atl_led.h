/**
 * @file atl_led.h
 * @author Robson Costa (robson.costa@ifsc.edu.br)
 * @brief LED header.
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
#include <inttypes.h>

#define LED_STRIP_RMT_RES_HZ  (10 * 1000 * 1000)

/**
 * @enum    atl_led_behaviour_e
 * @brief   LED behaviour.
 */
typedef enum {
    ATL_LED_DISABLED,
    ATL_LED_ENABLED_FAILS,
    ATL_LED_ENABLED_COMM_FAILS,
    ATL_LED_ENABLED_FULL,    
} atl_led_behaviour_e;

/**
 * @typedef atl_led_rgb_color_t
 * @brief System configuration structure.
 */
typedef struct {
    uint8_t red; /**< LED red value. */
    uint8_t green; /**< LED green value. */
    uint8_t blue; /**< LED blue value. */
} atl_led_rgb_color_t;

/**
 * @fn atl_led_builtin_init(void)
 * @brief Initialize led builtin task
 * @return Led strip config handle
*/
void atl_led_builtin_init(void);

/**
 * @fn atl_led_builtin_toogle(void)
 * @brief Toggle led builtin
*/
void atl_led_builtin_toogle(void);

/**
 * @fn atl_led_builtin_blink(uint8_t times, uint16_t interval, uint8_t red, uint8_t green, uint8_t blue)
 * @brief Blink led builtin
 * @param [in] times blink times
 * @param [in] interval interval between blinks
 * @param [in] red red value (0..255)
 * @param [in] green green value (0..255)
 * @param [in] blue blue value (0..255)
*/
void atl_led_builtin_blink(uint8_t times, uint16_t interval, uint8_t red, uint8_t green, uint8_t blue);

/**
 * @fn atl_led_set_color(uint8_t red, uint8_t green, uint8_t blue)
 * @brief Set led builtin color
 * @param [in] red red value (0..255)
 * @param [in] green green value (0..255)
 * @param [in] blue blue value (0..255)
*/
void atl_led_set_color(uint8_t red, uint8_t green, uint8_t blue);

/**
 * @fn atl_led_set_enabled(bool status)
 * @brief Enabled/disabled led builtin
 * @param [in] status enabled or disabled led builtin
*/
void atl_led_set_enabled(bool status);


#ifdef __cplusplus
}
#endif