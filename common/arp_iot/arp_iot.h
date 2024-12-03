#ifndef __ARP_IOT_H
#define __ARP_IOT_H
#include "esp_err.h"
#include "hal/gpio_types.h"
#include "esp_netif.h"
#include "lwip/netif.h" 
#include "setting_iot.h"
#include "cJSON.h"
#include "lwip/ip_addr.h"


typedef struct {
    char Device[50];
    char IP[16];
    uint8_t Mac[6];
    char Status[50];
    int Active;
} arp_t;

typedef struct {
    arp_t arp[ARP_TABLE_SIZE];
    int numRows;
} arp_table_json_t;

struct eth_addr; //khai báo lại từ thư viện khác
#define BUFFER_SIZE_CHECK_ARP 150  // Kích thước buffer (số lượng cặp IP có thể lưu)
typedef struct {
    ip4_addr_t src_ip;  // Địa chỉ IP nguồn
    ip4_addr_t dest_ip; // Địa chỉ IP đích
} ip_ring_buffer_t;


typedef void (*ethinterface_t)(); //tạo con trỏ hàm
typedef void (*checkstatus)(int i, int status);
typedef void (*checkarpscan)();

void sendArp_request(esp_netif_t *eth_netif);
void check_arp_status(esp_netif_t *eth_netif);
void printfArp();
void arp_scan_handle(int d);
void sendArp_keepConnected(esp_netif_t *eth_netif, struct eth_addr MAC1, ip4_addr_t IP1, struct eth_addr MAC2, ip4_addr_t IP2);
void keepConnected_callback(settings_t setting, esp_netif_t *eth_netif);

void input_netif_callback(void* cb);
void input_check_callback(void* cb);
void input_check_scan_callback(void* cb);
void sendArp_block(esp_netif_t *eth_netif, struct eth_addr target_mac, 
ip4_addr_t target_ip, struct eth_addr block_mac, ip4_addr_t block_ip);
void creatarp_table(esp_netif_t *eth_netif);
void check_arp_receive(struct netif *netif, const struct eth_addr *shwaddr, ip4_addr_t sipaddr, ip4_addr_t dipaddr);
void add_ip_ring_buffer(ip4_addr_t src_ip, ip4_addr_t dest_ip);
void send_arpupdate(cJSON *json);

extern void log_event(const char *event);

extern int index_default;
extern ip4_addr_t honeypot_ip;
extern int priority_main;
extern ip4_addr_t zero_ip;
extern bool alert;
extern TaskHandle_t xHandle_keep_connect;

extern volatile uint32_t delayTime_sendarp;
extern volatile uint32_t delayTime_checkStatus;
extern volatile uint32_t delayTime_checkArpScan;

extern bool suspendFlag_sendarp;
extern bool suspendFlag_checkStatus;
extern bool suspendFlag_checkArpScan;
extern bool suspendFlag_honeypot;
extern bool suspendFlag_keepConnect;
#endif
