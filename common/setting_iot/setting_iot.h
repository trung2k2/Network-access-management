#ifndef __SETTING_IOT_H
#define __SETTING_IOT_H
#include <stdio.h>
#include <stdbool.h>
#include "cJSON.h"
#include "lwip/netif.h" 
#include "esp_netif.h"
#include "lwip/ip_addr.h"
#include "esp_eth.h"
#include "esp_netif_net_stack.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "lwip/ip4.h"
#include "netif/ethernet.h"
#include "etharp_iot.h"
#include <time.h>

extern volatile uint32_t delayTime_sendarp; // Thời gian delay mặc định là 100 giây

void task_init_set(void);

#define MAX_ROWS 3 // Số lượng hàng tối đa trong bảng duy trì kết nối

typedef struct {
    char protocol[16];
    char IP1[16];
    char IP2[16];
    bool switchState;
} row_t;

// Lưu trữ cài đặt
typedef struct {
    bool scanArpEnabled;
    int scanArpInterval;
    bool honeypotEnabled;
    char honeypotIP[16];
    bool detectArpEnabled;
    int detectArpInterval;
    int differentIps;

    char userAdmin[16];
    char passAdmin[16];
    char userConsultant[16];
    char passConsultant[16];
    char codeGuest[9];

    row_t rows[MAX_ROWS];
    int numRows;

    char phone1[11];
    char phone2[11];
} settings_t;
//extern bien
extern ip4_addr_t honeypot_ip;
extern int honey;
extern int scan;
extern char correct_username[];
extern char correct_password[];
extern char correct_username_consultant[];
extern char correct_password_consultant[];
extern char correct_codeguest[];
extern int head;  // Vị trí của phần tử mới nhất sẽ được ghi vào
extern int tail;  // Vị trí của phần tử cũ nhất
extern int size;  // Kích thước thực tế của buffer (số lượng cặp IP hiện có)
extern bool alert;

void syslog_json2server(cJSON *json);
void send_json2server(cJSON *json);
typedef void (*ethinterface_t)(); //tạo con trỏ hàm
void input_get_netif_callback(void* cb);
void update_row_with_netif(esp_netif_t *netif_ether);
void format_time(time_t time, char *buffer, size_t size);
void input_get_netif_callback(void* cb);
time_t my_timegm(struct tm *tm);
void sync_time_with_client(struct tm client_tm);
void log_event(const char *event);
void display_logs();
void init_time() ;

extern void check_arp_scan_task(void *pvParameters);
extern void sendarp_task(void *pvParameters);
extern void check_status_task(void *pvParameters);
extern void input_check_scan_callback(void* cb);
extern void arp_scan_handle(int d);
extern void keepConnected_callback(settings_t setting, esp_netif_t *eth_netif);



#endif