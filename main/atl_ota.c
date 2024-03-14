/**
 * @file atl_ota.c
 * @author Robson Costa (robson.costa@ifsc.edu.br)
 * @brief OTA function.
 * @version 0.1.0
 * @date 2024-03-14 (created)
 * @date 2024-03-14 (updated)
 * 
 * @copyright Copyright &copy; since 2024 <a href="https://agrotechlab.lages.ifsc.edu.br" target="_blank">AgroTechLab</a>.\n
 * ![LICENSE license](../figs/license.png)<br>
 * Licensed under the CC BY-SA (<i>Creative Commons Attribution-ShareAlike</i>) 4.0 International Unported License (the <em>"License"</em>). You may not
 * use this file except in compliance with the License. You may obtain a copy of the License <a href="https://creativecommons.org/licenses/by-sa/4.0/legalcode" target="_blank">here</a>.
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an <em>"as is" basis, 
 * without warranties or  conditions of any kind</em>, either express or implied. See the License for the specific language governing permissions 
 * and limitations under the License.
 */
#include "atl_ota.h"

/* Constants */
static const char *TAG = "atl-ota"; /**< Function identification */
const char *atl_ota_behaviour_str[] = {
    "ATL_OTA_BEHAVIOUR_DISABLED",
	"ATL_OTA_BEHAVIOUR_VERIFY_NOTIFY",
	"ATL_OTA_BEHAVIOU_DOWNLOAD",
	"ATL_OTA_BEHAVIOU_DOWNLOAD_REBOOT",
};

/**
 * @brief Get the ota behaviour string object
 * @param behaviour 
 * @return Function enum const* 
 */
const char* atl_ota_get_behaviour_str(atl_ota_behaviour_e behaviour) {
    return atl_ota_behaviour_str[behaviour];
}