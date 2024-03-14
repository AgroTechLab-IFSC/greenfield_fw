/**
 * @file atl_mqtt.h
 * @author Robson Costa (robson.costa@ifsc.edu.br)
 * @brief MQTT header.
 * @version 0.1.0
 * @date 2024-03-13 (created)
 * @date 2024-03-13 (updated)
 * 
 * @copyright Copyright &copy; since 2024 <a href="https://agrotechlab.lages.ifsc.edu.br" target="_blank">AgroTechLab</a>.\n
 * ![LICENSE license](../figs/license.png)<br>
 * Licensed under the CC BY-SA (<i>Creative Commons Attribution-ShareAlike</i>) 4.0 International Unported License (the <em>"License"</em>). You may not
 * use this file except in compliance with the License. You may obtain a copy of the License <a href="https://creativecommons.org/licenses/by-sa/4.0/legalcode" target="_blank">here</a>.
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an <em>"as is" basis, 
 * without warranties or conditions of any kind</em>, either express or implied. See the License for the specific language governing permissions 
 * and limitations under the License.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef atl_mqtt_mode_e
 * @brief   MQTT mode.
 */
typedef enum {
    ATL_MQTT_DISABLED,
    ATL_MQTT_AGROTECHLAB_CLOUD,
    ATL_MQTT_THIRD,    
} atl_mqtt_mode_e;

/**
 * @typedef atl_mqtt_qos_e
 * @brief   MQTT QoS level.
 */
typedef enum {
    ATL_MQTT_QOS0,
    ATL_MQTT_QOS1,
    ATL_MQTT_QOS2,
} atl_mqtt_qos_e;

/**
 * @brief Get the MQTT mode string object
 * @param mode 
 * @return Function enum const* 
 */
const char* atl_mqtt_get_mode_str(atl_mqtt_mode_e mode);

/**
 * @brief Get the MQTT transport string object
 * @param transport 
 * @return Function enum const* 
 */
const char* atl_mqtt_get_transport_str(esp_mqtt_transport_t transport);

/**
 * @fn atl_mqtt_init(void)
 * @brief Initialize MQTT client service.
 */
void atl_mqtt_init(void);

#ifdef __cplusplus
}
#endif