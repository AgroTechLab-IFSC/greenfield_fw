/**
 * @file atl_dns.c
 * @author Robson Costa (robson.costa@ifsc.edu.br)
 * @brief DNS function.
 * @version 0.1.0
 * @date 2024-03-11 (created)
 * @date 2024-03-11 (updated)
 * 
 * @copyright Copyright &copy; since 2024 <a href="https://agrotechlab.lages.ifsc.edu.br" target="_blank">AgroTechLab</a>.\n
 * ![LICENSE license](../figs/license.png)<br>
 * Licensed under the CC BY-SA (<i>Creative Commons Attribution-ShareAlike</i>) 4.0 International Unported License (the <em>"License"</em>). You may not
 * use this file except in compliance with the License. You may obtain a copy of the License <a href="https://creativecommons.org/licenses/by-sa/4.0/legalcode" target="_blank">here</a>.
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an <em>"as is" basis, 
 * without warranties or conditions of any kind</em>, either express or implied. See the License for the specific language governing permissions 
 * and limitations under the License.
 */
#include <sys/param.h>
#include <inttypes.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_system.h>
#include <esp_netif.h>
#include <lwip/err.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/netdb.h>
#include "atl_dns.h"

/* Constants */
static const char *TAG = "atl-dns";

/* Global variables */
TaskHandle_t atl_dns_handle = NULL; /**< DNS task handle */

/**
 * @fn parse_dns_name(char *raw_name, char *parsed_name, size_t parsed_name_max_len)
 * @brief Parse DNS name
 * @details Parse the name from the packet from the DNS name format to a regular .-seperated
 *  name returns the pointer to the next part of the packet
 * @param[in] raw_name - raw name
 * @param[out] parsed_name - parsed name
 * @param[in] parsed_name_max_len - maximum size of parsed name
*/
static char *parse_dns_name(char *raw_name, char *parsed_name, size_t parsed_name_max_len) {

    char *label = raw_name;
    char *name_itr = parsed_name;
    int name_len = 0;

    do {
        int sub_name_len = *label;
        /* (len + 1) since we are adding  a '.' */
        name_len += (sub_name_len + 1);
        if (name_len > parsed_name_max_len) {
            return NULL;
        }

        /* Copy the sub name that follows the the label */
        memcpy(name_itr, label + 1, sub_name_len);
        name_itr[sub_name_len] = '.';
        name_itr += (sub_name_len + 1);
        label += sub_name_len + 1;
    } while (*label != 0);

    /* Terminate the final string, replacing the last '.' */
    parsed_name[name_len - 1] = '\0';
    /* Return pointer to first char after the name */
    return label + 1;
}

/**
 * @fn parse_dns_request(char *req, size_t req_len, char *dns_reply, size_t dns_reply_max_len)
 * @brief Parse DNS name and reply with softAP IP
 * @details Parses the DNS request and prepares a DNS response with the IP of the softAP
 * @param[in] req - request
 * @param[in] req_len - request length
 * @param[out] dns_reply - DNS reply
 * @param[out] dns_reply_max_len - maximum size of DNS reply
*/
static int parse_dns_request(char *req, size_t req_len, char *dns_reply, size_t dns_reply_max_len) {
    if (req_len > dns_reply_max_len) {
        return -1;
    }

    /* Prepare the reply */
    memset(dns_reply, 0, dns_reply_max_len);
    memcpy(dns_reply, req, req_len);

    /* Endianess of NW packet different from chip */
    dns_header_t *header = (dns_header_t *)dns_reply;
    ESP_LOGD(TAG, "DNS query with header id: 0x%X, flags: 0x%X, qd_count: %d",
             ntohs(header->id), ntohs(header->flags), ntohs(header->qd_count));

    /* Not a standard query */
    if ((header->flags & OPCODE_MASK) != 0) {
        return 0;
    }

    /* Set question response flag */
    header->flags |= QR_FLAG;

    uint16_t qd_count = ntohs(header->qd_count);
    header->an_count = htons(qd_count);

    int reply_len = qd_count * sizeof(dns_answer_t) + req_len;
    if (reply_len > dns_reply_max_len) {
        return -1;
    }

    /* Pointer to current answer and question */
    char *cur_ans_ptr = dns_reply + req_len;
    char *cur_qd_ptr = dns_reply + sizeof(dns_header_t);
    char name[128];

    /* Respond to all questions with the softAP IP address */
    for (int i = 0; i < qd_count; i++) {
        char *name_end_ptr = parse_dns_name(cur_qd_ptr, name, sizeof(name));
        if (name_end_ptr == NULL) {
            ESP_LOGE(TAG, "Failed to parse DNS question: %s", cur_qd_ptr);
            return -1;
        }

        dns_question_t *question = (dns_question_t *)(name_end_ptr);
        uint16_t qd_type = ntohs(question->type);
        uint16_t qd_class = ntohs(question->class);

        ESP_LOGD(TAG, "Received type: %d | Class: %d | Question for: %s", qd_type, qd_class, name);

        if (qd_type == QD_TYPE_A) {
            dns_answer_t *answer = (dns_answer_t *)cur_ans_ptr;

            answer->ptr_offset = htons(0xC000 | (cur_qd_ptr - dns_reply));
            answer->type = htons(qd_type);
            answer->class = htons(qd_class);
            answer->ttl = htonl(ANS_TTL_SEC);

            esp_netif_ip_info_t ip_info;
            esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);
            ESP_LOGD(TAG, "Answer with PTR offset: 0x%" PRIX16 " and IP 0x%" PRIX32, ntohs(answer->ptr_offset), ip_info.ip.addr);

            answer->addr_len = htons(sizeof(ip_info.ip.addr));
            answer->ip_addr = ip_info.ip.addr;
        }
    }
    return reply_len;
}

/**
 * @fn atl_dns_server_task(void *pvParameters)
 * @brief DNS server task
 * @details This DNS server replies to all type A queries with the IP of the softAP
 * @param[in] pvParameters - pointer to parameters
*/
static void atl_dns_server_task(void *pvParameters) {
    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    /* Request process loop */
    while (1) {

        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(DNS_PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket bound, port %d", DNS_PORT);

        while (1) {
            ESP_LOGD(TAG, "Waiting for data");
            struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
            socklen_t socklen = sizeof(source_addr);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

            /* Error occurred during receiving */
            if (len < 0) {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                close(sock);
                break;
            }
            /* Data received */
            else {
                /* Get the sender's ip address as string */
                if (source_addr.sin6_family == PF_INET) {
                    inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                } else if (source_addr.sin6_family == PF_INET6) {
                    inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
                }

                /* Null-terminate whatever we received and treat like a string... */
                rx_buffer[len] = 0;

                char reply[DNS_MAX_LEN];
                int reply_len = parse_dns_request(rx_buffer, len, reply, DNS_MAX_LEN);

                ESP_LOGD(TAG, "Received %d bytes from %s | DNS reply with len: %d", len, addr_str, reply_len);
                if (reply_len <= 0) {
                    ESP_LOGE(TAG, "Failed to prepare a DNS reply");
                } else {
                    int err = sendto(sock, reply, reply_len, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
                    if (err < 0) {
                        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                        break;
                    }
                }
            }
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

/**
 * @fn atl_dns_server_init(void)
 * @brief Initialize DNS capture server.
 * @details Initialize DNS server that replies to all type A queries with the IP of the softAP.
 */
void atl_dns_server_init(void) {
    ESP_LOGI(TAG, "Initializing DNS server!");    
    xTaskCreatePinnedToCore(atl_dns_server_task, "atl_dns_task", 4096, NULL, 10, &atl_dns_handle, 1);
}