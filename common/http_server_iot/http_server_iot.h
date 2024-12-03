#ifndef __HTTP_SERVER_IOT_H
#define __HTTP_SERVER_IOT_H
#include "cJSON.h"
#include <time.h>
#include <lwip/sockets.h>

typedef struct {
    const char* name;
    const uint8_t* start;
    const uint8_t* end;
} image_info_t;



typedef void (*http_post_callback_t)(char* data, int len);
typedef void (*http_get_callback_t)(char* data, int len, cJSON *json);
void start_webserver(void);
void stop_webserver(void);
void http_set_callback_switch(void *cb);
void http_set_callback_setting(void *cb);
extern ip4_addr_t zero_ip;
extern time_t initial_time;

extern void display_logs();
extern void trust_sign(ip4_addr_t ip_check, const char *role);
extern void send_json2server(cJSON *json);
extern void syslog_json2server(cJSON *json);
extern void send_arpupdate(cJSON *json);
extern void format_time(time_t time, char *buffer, size_t size);
extern void input_get_netif_callback(void* cb);
extern time_t my_timegm(struct tm *tm);
extern void sync_time_with_client(struct tm client_tm);
extern ip4_addr_t get_client_ip(int client_socket);
#endif