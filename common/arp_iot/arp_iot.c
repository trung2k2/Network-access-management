#include <stdio.h>
#include <esp_log.h>
#include "arp_iot.h"
#include "esp_attr.h"
#include "etharp_iot.h"
#include "lwip/err.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "lwip/ip4.h"
#include "netif/ethernet.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/sys.h"
#include "esp_netif_net_stack.h"
#include "driver/gpio.h"
#include "lwip/ip_addr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "arp_iot.h"
#include "setting_iot.h"
#include "cJSON.h"


ethinterface_t eth_netif_callback = NULL;
ethinterface_t check_callback = NULL;
checkarpscan check_scan_callback = NULL;

ip4_addr_t honeypot_ip;
ip4_addr_t dhcp_server_ip;
ip4_addr_t relevant_ips[2]; // Mảng lưu IP nguồn vượt quá điều kiện
int relevant_count = 0;

int head = 0;  // Vị trí của phần tử mới nhất sẽ được ghi vào
int tail = 0;  // Vị trí của phần tử cũ nhất
int size = 0;  // Kích thước thực tế của buffer (số lượng cặp IP hiện có)
ip_ring_buffer_t ip_buffer[BUFFER_SIZE_CHECK_ARP];

arp_table_json_t arp_table_json = {
    .arp = {},
    .numRows = 0,
};
char logevent[200];

//luu goi tin vao ring buffer
void add_ip_ring_buffer(ip4_addr_t src_ip, ip4_addr_t dest_ip) {
    ip_buffer[head].src_ip = src_ip;
    ip_buffer[head].dest_ip = dest_ip;
    
    // Tăng vị trí head (vòng tròn)
    head = (head + 1) % BUFFER_SIZE_CHECK_ARP;

    // Nếu buffer đầy, di chuyển tail (ghi đè phần tử cũ nhất)
    if (size == BUFFER_SIZE_CHECK_ARP) {
        tail = (tail + 1) % BUFFER_SIZE_CHECK_ARP;
    } else {
        size++;
    }
}
int scan =0;

//ham doc ring buffer, tùy theo thời gian querry interval để gọi
void arp_scan_handle(int d) {
    relevant_count = 0; // Đặt lại đếm số IP nguồn

    for (int i = 0; i < size; i++) {
        int index = (tail + i) % BUFFER_SIZE_CHECK_ARP; // Tính chỉ số theo kiểu vòng tròn
        ip4_addr_t current_src_ip = ip_buffer[index].src_ip;
        //ip4_addr_t current_dest_ip = ip_buffer[index].dest_ip;

        // Kiểm tra xem current_src_ip đã được lưu chưa (số ip_dest vượt quá số lượng cho phép)
        int found = 0;
        for (int j = 0; j < relevant_count; j++) {
            if (ip4_addr_cmp(&relevant_ips[j], &current_src_ip)) {
                found = 1; // Đã tìm thấy IP này
                break;
            }
        }

        // Nếu chưa tìm thấy, kiểm tra số dest_ip khác nhau
        if (!found) {
            ip4_addr_t *seen_dest_ips = (ip4_addr_t *)malloc(sizeof(ip4_addr_t) * BUFFER_SIZE_CHECK_ARP);
                if (seen_dest_ips == NULL) {
                    // Xử lý lỗi không cấp phát được bộ nhớ
                    return;
                }
            int seen_count = 0; // Đếm số lượng dest_ip khác nhau

            // Quét lại toàn bộ buffer để đếm dest_ip cho current_src_ip
            for (int k = 0; k < size; k++) {
                int inner_index = (tail + k) % BUFFER_SIZE_CHECK_ARP; // Chỉ số theo kiểu vòng tròn
                if (ip4_addr_cmp(&ip_buffer[inner_index].src_ip, &current_src_ip)) {
                    // Nếu dest_ip chưa thấy, lưu vào mảng
                    int is_new_dest_ip = 1;
                    for (int l = 0; l < seen_count; l++) {
                        if (ip4_addr_cmp(&seen_dest_ips[l], &ip_buffer[inner_index].dest_ip)) {
                            is_new_dest_ip = 0; // Đã thấy dest_ip này
                            break;
                        }
                    }

                    // Nếu là dest_ip mới, tăng số lượng đã thấy
                    if (is_new_dest_ip) {
                        seen_dest_ips[seen_count++] = ip_buffer[inner_index].dest_ip;
                    }
                }
            }

            // Nếu số lượng dest_ip khác nhau vượt quá d, lưu current_src_ip vào mảng
            if (seen_count > d && relevant_count < 2) {
                relevant_ips[relevant_count++] = current_src_ip;
            }
            free(seen_dest_ips);
        }
        
    }
    if(relevant_count != 0 && scan == 0) {
        for (int i = 0; i < arp_table_index; i++) 
        {
        if(ip4_addr_cmp(&relevant_ips[0], &custom_arp_table[i].ip))
        {
            custom_arp_table[i].active = 1;
            if(honey==1)
            memcpy(custom_arp_table[i].status, "Scan-detected, Honeypot - hit", strlen("Scan-detected, Honeypot - hit") + 1);
            else
            memcpy(custom_arp_table[i].status, "Scan-detected", strlen("Scan-detected") + 1);
            const char *ip_str = ip4addr_ntoa(&custom_arp_table[i].ip);
            snprintf(logevent, sizeof(logevent), 
                    "One device scan network has detected- IP: %s MAC: %02X:%02X:%02X:%02X:%02X:%02X", 
                    ip_str,
                    custom_arp_table[i].mac[0], custom_arp_table[i].mac[1], custom_arp_table[i].mac[2],
                    custom_arp_table[i].mac[3], custom_arp_table[i].mac[4], custom_arp_table[i].mac[5]);
            printf("Src IP scan: %s\n", ip4addr_ntoa(&custom_arp_table[i].ip));
            log_event(logevent);
            printf("Src IP scan: %s\n", ip4addr_ntoa(&relevant_ips[0]));
            
            if(!alert)
            {
                vTaskResume(xHandle_keep_connect);
                alert = true;
            }
            
        }
        }
        scan =1;
    }   
}

void check_scan()
{
    check_scan_callback();
}

//tạo task check ring buffer 30s một lần
void check_arp_scan_task(void *pvParameters) {
    while (1) {
        if (priority_main && suspendFlag_checkArpScan) {
            check_scan();
            vTaskDelay(delayTime_checkArpScan / portTICK_PERIOD_MS); // Delay 30 giây
        }
        else {          
            vTaskSuspend(NULL);  // Suspend chính task hiện tại
        }
    }
}

// thuc hien quet ARP
void sendArp_request(esp_netif_t *eth_netif)
{
    struct netif *lwip_netif = esp_netif_get_netif_impl(eth_netif);
    ip4_addr_t ipser;
    ip4_addr_t current_ip = lwip_netif->ip_addr.u_addr.ip4;
    uint8_t first_octet = ip4_addr1(&current_ip);
    uint8_t second_octet = ip4_addr2(&current_ip);
    uint8_t third_octet = ip4_addr3(&current_ip);
    uint8_t new_octet4;
    //hàm gui arp
     for(int i=0;i<=255;i++)
     {
     new_octet4 = i;
     IP4_ADDR(&ipser, first_octet, second_octet, third_octet, new_octet4);
     etharp_request(lwip_netif, &ipser);
     vTaskDelay(1 / portTICK_PERIOD_MS); //thoi gian giua hai goi arp quet
     }
}
//ham tạo file json gửi đến client
uint8_t MAC_Seimens[3] = {0xE0,0xDC,0xA0};
uint8_t MAC_Tplink[3] = {0xC0,0x4A,0x00};

void send_arpupdate(cJSON *json)
{
    // Tạo mảng JSON cho các hàng (rows)
    cJSON *rowsArray = cJSON_CreateArray();
    char Device[100];
    cJSON_AddNumberToObject(json, "numRows", arp_table_index);
    for (int i = 0; i < arp_table_index; i++) { 
        cJSON *rowObject = cJSON_CreateObject();
        int switchId = i;
        if(memcmp(custom_arp_table[i].mac, MAC_Seimens, 3)==0)
        {
            memcpy(Device, "SIEMENS", strlen("SIEMENS") + 1);
        }
        else if(memcmp(custom_arp_table[i].mac, MAC_Tplink, 3)==0)
        {
            memcpy(Device, "TP-Link", strlen("TP-Link") + 1);
        }
        else
        memcpy(Device, "Unknow", strlen("Unknow") + 1);
        cJSON_AddStringToObject(rowObject, "device", Device);
        cJSON_AddStringToObject(rowObject, "ip_address", ip4addr_ntoa(&custom_arp_table[i].ip));
        // Thêm thông tin địa chỉ MAC (chuyển đổi thành chuỗi)
        char mac_address[18];
        snprintf(mac_address, sizeof(mac_address), "%02X:%02X:%02X:%02X:%02X:%02X",
                custom_arp_table[i].mac[0], custom_arp_table[i].mac[1], custom_arp_table[i].mac[2],
                custom_arp_table[i].mac[3], custom_arp_table[i].mac[4], custom_arp_table[i].mac[5]);
        cJSON_AddStringToObject(rowObject, "mac_address", mac_address);
        cJSON_AddStringToObject(rowObject, "status", custom_arp_table[i].status);
        cJSON_AddNumberToObject(rowObject, "switch_id", switchId);
        cJSON_AddBoolToObject(rowObject, "active", custom_arp_table[i].active == 1);

        cJSON_AddItemToArray(rowsArray, rowObject);        
    }
    cJSON *rowObject2 = cJSON_CreateObject();
    if(scan == 1 && honey == 0)
    {        
        cJSON_AddStringToObject(rowObject2, "warningscan", "Scan");
        cJSON_AddItemToArray(rowsArray, rowObject2); 
    }      
    else if(honey == 1 && scan == 0) 
    {
        cJSON_AddStringToObject(rowObject2, "warningscan", "");
        cJSON_AddStringToObject(rowObject2, "warninghoneypot", "Honeypot-hit");
        cJSON_AddItemToArray(rowsArray, rowObject2);    
    }     
    else if(honey == 1 && scan == 1) 
    {
        cJSON_AddStringToObject(rowObject2, "warningscan", "Scan");
        cJSON_AddStringToObject(rowObject2, "warninghoneypot", "Honeypot-hit");
        cJSON_AddItemToArray(rowsArray, rowObject2); 
    }
    else
    {
        cJSON_AddStringToObject(rowObject2, "warningscan", "");
        cJSON_AddStringToObject(rowObject2, "warninghoneypot", "");
        cJSON_AddItemToArray(rowsArray, rowObject2); 
    }
    cJSON_AddItemToObject(json, "arp", rowsArray);
    


}

struct eth_addr mac_broadcast = {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
//target là các thiết bị trong mạng
//block là thiết bị bị chặn
void sendArp_block(esp_netif_t *eth_netif, struct eth_addr target_mac, ip4_addr_t target_ip, struct eth_addr block_mac, ip4_addr_t block_ip)
{
    //printf("Thuc hien block\n");
    struct netif *lwip_netif = esp_netif_get_netif_impl(eth_netif);
    etharp_raw(lwip_netif,
                (struct eth_addr *)lwip_netif->hwaddr, &target_mac,
                (struct eth_addr *)lwip_netif->hwaddr, &block_ip,
                &mac_broadcast, &block_ip, ARP_REQUEST);
    etharp_raw(lwip_netif,
                (struct eth_addr *)lwip_netif->hwaddr, &target_mac,
                (struct eth_addr *)lwip_netif->hwaddr, &block_ip,
                &mac_broadcast, &block_ip, ARP_REPLY);
    etharp_raw(lwip_netif,
                (struct eth_addr *)lwip_netif->hwaddr, &target_mac,
                (struct eth_addr *)lwip_netif->hwaddr, &block_ip,
                &target_mac, &target_ip, ARP_REPLY);
    etharp_raw(lwip_netif,
                (struct eth_addr *)lwip_netif->hwaddr, &block_mac,
                (struct eth_addr *)lwip_netif->hwaddr, &target_ip,
                &block_mac, &block_ip, ARP_REPLY);
}

//Duy tri ket noi IP1, MAC1, IP2, MAC2
void sendArp_keepConnected(esp_netif_t *eth_netif, struct eth_addr MAC1, ip4_addr_t IP1, struct eth_addr MAC2, ip4_addr_t IP2)
{
    struct netif *lwip_netif = esp_netif_get_netif_impl(eth_netif);
    etharp_raw(lwip_netif,
                &MAC1, &MAC2,
                &MAC1, &IP1,
                &MAC2, &IP2, ARP_REPLY);
    etharp_raw(lwip_netif,
                &MAC2, &MAC1,
                &MAC2, &IP2,
                &MAC1, &IP1, ARP_REPLY);
}

void keepConnected_callback(settings_t setting, esp_netif_t *eth_netif)
{
    ip4_addr_t IP1[3], IP2[3];
    struct eth_addr MAC1[3], MAC2[3];
    for (int i = 0; i < 3; i++) {
    IP4_ADDR(&IP1[i], 0, 0, 0, 0);
    IP4_ADDR(&IP2[i], 0, 0, 0, 0);
}
   
    for(int i =0; i < setting.numRows; i++)
    {
        if(setting.rows[i].switchState)
        {
            ip4addr_aton(setting.rows[i].IP1, &IP1[i]);
            ip4addr_aton(setting.rows[i].IP2, &IP2[i]);
        }
    }
    // So sánh địa chỉ IP và sao chép MAC nếu khớp
    for (int i = 0; i < setting.numRows; i++)
    {
        for (int j = 0; j < arp_table_index; j++)
        {
            if (ip4_addr_cmp(&IP1[i], &custom_arp_table[j].ip))
            {
                memcpy(&MAC1[i], custom_arp_table[j].mac, sizeof(MAC1[i]));
            }
            if (ip4_addr_cmp(&IP2[i], &custom_arp_table[j].ip))
            {
                memcpy(&MAC2[i], custom_arp_table[j].mac, sizeof(MAC2[i]));
            }
        }
    }
    IP4_ADDR(&zero_ip, 0, 0, 0, 0);
    for(int i =0; i < setting.numRows; i++)
    {
        if(!ip4_addr_cmp(&IP1[i], &zero_ip))
        {
            printf("IP1%d: %s, MAC1%d: %02X:%02X:%02X:%02X:%02X:%02X\n", 
            i + 1, ip4addr_ntoa(&IP1[i]), i + 1, 
            MAC1[i].addr[0], MAC1[i].addr[1], MAC1[i].addr[2], 
            MAC1[i].addr[3], MAC1[i].addr[4], MAC1[i].addr[5]);
            printf("IP2%d: %s, MAC2%d: %02X:%02X:%02X:%02X:%02X:%02X\n", 
            i + 1, ip4addr_ntoa(&IP2[i]), i + 1, 
            MAC2[i].addr[0], MAC2[i].addr[1], MAC2[i].addr[2], 
            MAC2[i].addr[3], MAC2[i].addr[4], MAC2[i].addr[5]);
            
            sendArp_keepConnected(eth_netif, MAC1[i], IP1[i], MAC2[i], IP2[i]);
        }
        
    }
}

void check_arp_status(esp_netif_t *eth_netif)
{
    struct eth_addr eth_mac_block;
    struct eth_addr eth_mac;
    for (int i = 0; i < arp_table_index; i++) 
    {
        if(custom_arp_table[i].active==1)
        {
            memcpy(eth_mac_block.addr, custom_arp_table[i].mac, sizeof(eth_mac_block.addr));
            for (int j = 0; j < arp_table_index; j++) 
            {
                if(i != j && !ip4_addr_cmp(&custom_arp_table[j].ip, &dhcp_server_ip))
                {
                memcpy(eth_mac.addr, custom_arp_table[j].mac, sizeof(eth_mac.addr));
                sendArp_block(eth_netif, eth_mac, custom_arp_table[j].ip, eth_mac_block, custom_arp_table[i].ip);
                }
            }
    }
}}

// Task để cập nhật bảng mỗi 100 giây
void update_arp_data() {
    eth_netif_callback();
}

void sendarp_task(void *pvParameters) {
    while(1){
        if (priority_main && suspendFlag_sendarp) {
            //printf("Bat task\n");                
                update_arp_data();

            vTaskDelay(delayTime_sendarp/ portTICK_PERIOD_MS); // Delay 100 giây            
        }
        else {   
            //printf("Tat task\n");     
            vTaskSuspend(NULL);  // Suspend chính task hiện tại
        }
    }

}
// Task kiểm tra trạng thái mỗi 10 giây và thực hiện block
void block_arp() {
    check_callback();
}
void check_status_task(void *pvParameters) { //kiểm tra xem có status bằng 1 thì chặn
    while (1)
    {
        if (priority_main){
            block_arp();
           
        } 
         vTaskDelay(delayTime_checkStatus / portTICK_PERIOD_MS); // Delay 10 giây
    }
}


//Ham kiem tra honeypot
int honey = 0; //bien check trạng thái honeypot
void check_arp_receive(struct netif *netif, const struct eth_addr *shwaddr, ip4_addr_t sipaddr, ip4_addr_t dipaddr)
{
    for (int i = 0; i < arp_table_index; i++) 
        {
        if(ip4_addr_cmp(&sipaddr, &custom_arp_table[i].ip)||ip4_addr_cmp(&dipaddr, &custom_arp_table[i].ip))
        {
          if(custom_arp_table[i].active==1)
            {
                etharp_raw(netif,
                      (struct eth_addr *)netif->hwaddr, shwaddr,
                      (struct eth_addr *)netif->hwaddr, &dipaddr,
                      shwaddr, &sipaddr,
                      ARP_REPLY);
            }
          if(ip4_addr_cmp(&dipaddr, &honeypot_ip) && honey == 0 && suspendFlag_honeypot)
            {
                if(scan==0)
                memcpy(custom_arp_table[i].status, "Honeypot - hit", strlen("Honeypot - hit") + 1); // Điền trạng thái mới
                else
                memcpy(custom_arp_table[i].status, "Honeypot - hit, Scan-detected", strlen("Honeypot - hit, Scan-detected") + 1); // Điền trạng thái mới
                custom_arp_table[i].active = 1;
                const char *ip_str = ip4addr_ntoa(&custom_arp_table[i].ip);
                snprintf(logevent, sizeof(logevent), 
                        "One device tried to connect to the honeypot - IP: %s MAC: %02X:%02X:%02X:%02X:%02X:%02X", 
                        ip_str,
                        custom_arp_table[i].mac[0], custom_arp_table[i].mac[1], custom_arp_table[i].mac[2],
                        custom_arp_table[i].mac[3], custom_arp_table[i].mac[4], custom_arp_table[i].mac[5]);
                //printf("Src IP honeypot: %s\n", ip4addr_ntoa(&custom_arp_table[i].ip));
                log_event(logevent);
                if(!alert)
                {
                    alert = true;
                    vTaskResume(xHandle_keep_connect);
                }
                honey = 1;
            }  
        }
        }
        
}


void creatarp_table(esp_netif_t *eth_netif)
{
    sendArp_request(eth_netif);
    for (int i = 0; i < arp_table_index; i++) { 
        custom_arp_table[i].active = 0;
        memcpy(custom_arp_table[i].status, "Trust", strlen("Trust") + 1);
    }
    priority_main = 1;
}
int index_default = 0;
void printfArp()
{   
    index_default = arp_table_index;
    printf("Bang ARP Table: %d\n",arp_table_index);
    for (int i = 0; i < arp_table_index; i++) {
        printf("IP: %s, MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
               ip4addr_ntoa(&custom_arp_table[i].ip),
               custom_arp_table[i].mac[0], custom_arp_table[i].mac[1], custom_arp_table[i].mac[2],
               custom_arp_table[i].mac[3], custom_arp_table[i].mac[4], custom_arp_table[i].mac[5]);
    }
}
int check_time_sign = 0;
void trust_sign(ip4_addr_t ip_check, const char *role)
{
    for (int i = 0; i < arp_table_index; i++)
        {
            if (ip4_addr_cmp(&ip_check, &custom_arp_table[i].ip))
            {
                
                const char *ip_str = ip4addr_ntoa(&custom_arp_table[i].ip);
                //printf("Role: %s\n", role);
                if(strcmp(role, "consultant") == 0 && check_time_sign != i)
                {
                    memcpy(custom_arp_table[i].status, "Consultant", strlen("Consultant") + 1); // Điền trạng thái mới
                    
                    snprintf(logevent, sizeof(logevent), 
                        "One device join as Consultant- IP: %s MAC: %02X:%02X:%02X:%02X:%02X:%02X", 
                        ip_str,
                        custom_arp_table[i].mac[0], custom_arp_table[i].mac[1], custom_arp_table[i].mac[2],
                        custom_arp_table[i].mac[3], custom_arp_table[i].mac[4], custom_arp_table[i].mac[5]);
                        //printf("Src IP honeypot: %s\n", ip4addr_ntoa(&custom_arp_table[i].ip));
                        log_event(logevent);
                        check_time_sign = i;
                }
                else if (strcmp(role, "admin") == 0)
                {
                    memcpy(custom_arp_table[i].status, "Trusted by admin", strlen("Trusted by admin") + 1); // Điền trạng thái mới
                    custom_arp_table[i].active = 0;
                }
                if (strcmp(role, "guest") == 0)
                {
                    if (!strstr(custom_arp_table[i].status, "Honeypot - hit") && !strstr(custom_arp_table[i].status, "Scan-detected"))
                    {
                        custom_arp_table[i].active = 0;
                        memcpy(custom_arp_table[i].status, "Trust", strlen("Trust") + 1); // Điền trạng thái mới
                        
                        snprintf(logevent, sizeof(logevent), 
                        "One device join with guest code - IP: %s MAC: %02X:%02X:%02X:%02X:%02X:%02X", 
                        ip_str,
                        custom_arp_table[i].mac[0], custom_arp_table[i].mac[1], custom_arp_table[i].mac[2],
                        custom_arp_table[i].mac[3], custom_arp_table[i].mac[4], custom_arp_table[i].mac[5]);
                        //printf("Src IP honeypot: %s\n", ip4addr_ntoa(&custom_arp_table[i].ip));
                        log_event(logevent);
                    }
                }
            }
        }
}


void input_netif_callback(void* cb) //gọi từ ngoài main truyền hàm cb vào con trỏ hàm
{   
    eth_netif_callback = cb; //truyền hàm vào con trỏ hàm
}

void input_check_callback(void* cb) //gọi từ ngoài main truyền hàm cb vào con trỏ hàm
{   
    check_callback = cb; //truyền hàm vào con trỏ hàm
}

void input_check_scan_callback(void* cb) //gọi từ ngoài main truyền hàm cb vào con trỏ hàm
{   
    check_scan_callback = cb; //truyền hàm vào con trỏ hàm
}

// void input_check_scan_callback(void* cb) //gọi từ ngoài main truyền hàm cb vào con trỏ hàm
// {   
//     keep_connected_callback = cb; //truyền hàm vào con trỏ hàm
// }