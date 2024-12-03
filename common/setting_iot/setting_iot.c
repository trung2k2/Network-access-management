#include "setting_iot.h"
#include <esp_wifi.h>
#include "arp_iot.h"
#include "http_server_iot.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>
#include <time.h>
#include "esp_timer.h"
#include <lwip/sockets.h>

static const char *TAG = "Setting";
TaskHandle_t xHandle_check_status = NULL;
TaskHandle_t xHandle_sendarp = NULL;
TaskHandle_t xHandle_check_arp_scan = NULL;
TaskHandle_t xHandle_keep_connect = NULL;

ethinterface_t get_netif_callback = NULL;
volatile uint32_t delayTime_sendarp = 100000; // Thời gian delay mặc định là 100 giây
volatile uint32_t delayTime_checkStatus = 10000; // Thời gian delay mặc định là 10 giây
volatile uint32_t delayTime_checkArpScan = 30000; // Thời gian delay mặc định là 30 giây
volatile int differentIP = 30;

//khoi tao trang thai cac task
bool suspendFlag_sendarp = true;
bool suspendFlag_checkStatus = true;
bool suspendFlag_checkArpScan = true;
bool suspendFlag_honeypot = true;
bool suspendFlag_keepConnect = true;

#define MAX_LOGS 100  // Giới hạn số lượng log lưu trữ

typedef struct {
    char event[200];
    time_t timestamp;  // Thời gian lưu log
} LogEntry;

LogEntry logs[MAX_LOGS];  // Mảng lưu log
int log_count = 0;  // Số lượng log hiện có
time_t initial_time;


// Khởi tạo giá trị ban đầu
settings_t settings = {
    //Advanced
    .scanArpEnabled = true,
    .scanArpInterval = 100,
    .honeypotEnabled = true,
    .honeypotIP = "192.168.0.161",
    .detectArpEnabled = true,
    .detectArpInterval = 30,
    .differentIps = 30,
    //Account
    .userAdmin = "admin",
    .passAdmin = "123456",
    .userConsultant = "consultant",
    .passConsultant = "123456",
    .codeGuest = "",

    //Aleart
    .rows = {
        //{ "TCP", "192.168.0.1", "192.168.0.2", true },
        // Các hàng khác
    },
    .numRows = 0,
    .phone1 = "",
    .phone2 = ""
};


// Hàm để resume và thay đổi thời gian delay của task
// Hàm để suspend task
void suspendTask(TaskHandle_t xHandle, bool suspendFlag) {
    if (xHandle != NULL && !suspendFlag) {
        vTaskSuspend(xHandle);  // Tạm dừng task
    }
}

// Hàm resume và set timedelay task
void resumeTask(TaskHandle_t xHandle, int newTime, volatile uint32_t *task_delayTime) {
    if (xHandle != NULL) {
        *task_delayTime = newTime;
        vTaskResume(xHandle);  // Tiếp tục task đã bị tạm dừng
    }
}

void update_setting(void)
{
    //Xu ly tuy chon quet ARP
    if(settings.scanArpEnabled)
        {
            suspendFlag_sendarp = true;
            resumeTask(xHandle_sendarp, settings.scanArpInterval*1000, &delayTime_sendarp);    
        }
    else if(!settings.scanArpEnabled)
        {
            suspendFlag_sendarp = false;
            vTaskSuspend(xHandle_sendarp);
        }
    //Xu ly tuy chọn honeypot
    if(settings.honeypotEnabled)
        {            
            suspendFlag_honeypot = true;
            ip4addr_aton(settings.honeypotIP, &honeypot_ip);
           
        }
    else if(!settings.honeypotEnabled)
        {
            suspendFlag_honeypot = false;
            honey = 0; 
            alert = false;
        }
    
    //Xu ly tuy chon phat hien Scan
    if(settings.detectArpEnabled)
        {
            resumeTask(xHandle_check_arp_scan, settings.detectArpInterval*1000, &delayTime_checkArpScan);
            suspendFlag_checkArpScan = true;
            differentIP = settings.differentIps;
            
        }
    else if(!settings.detectArpEnabled)
        {
            suspendFlag_checkArpScan = false;
            scan = 0;
            alert = false;
            //vTaskSuspend(xHandle_check_arp_scan);
        }
    
    //Cap nhat account
    strcpy(correct_password, settings.passAdmin);
    strcpy(correct_username, settings.userAdmin);
    strcpy(correct_password_consultant, settings.passConsultant);
    strcpy(correct_username_consultant, settings.userConsultant);
    strcpy(correct_codeguest, settings.codeGuest);
            //kichs hoat ngat
            //void 
    if(settings.numRows!=0)
    {
        suspendFlag_keepConnect = true;
        
    }
    else if(settings.numRows==0)
    {
        suspendFlag_keepConnect = false;
        vTaskSuspend(xHandle_keep_connect);
    }
}

/* Gói duy trì và callback*/
void update_row_with_netif(esp_netif_t *netif_ether)
{
    //printf("Goi ham kepp\n");
    keepConnected_callback(settings, netif_ether);
}

void update_row(void)
{
    get_netif_callback();
}   

//task gửi gói duy trì 
void keepConnected_task(void *pvParameters) {
    
    while (1) {  // Task sẽ chạy liên tục, không kết thúc.
        if (suspendFlag_keepConnect && alert) {
            printf("Bat task kepp\n");
            // Chạy khi flag đang được bật  
            update_row();
            vTaskDelay(5000 / portTICK_PERIOD_MS); // Delay 5 giây
        } else {          
            printf("Tat task kepp\n");
            vTaskSuspend(NULL);  // Suspend chính task hiện tại
        }
    }
}

//Ham xử lý goi json nhan duoc tu Client gui den
void setting_data_callback(char *data, int len, cJSON *json)
{
    printf("DATA: %s\n", data); //kiem tra du lieu nhan duoc
    if (json == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return;
    }

    /* <<<<<<<<<<<<<<<<<<<<<<<<< Trang Advanced Setting >>>>>>>>>>>>>>>>>>>>>>>>>  */

    cJSON *scanArpEnabled = cJSON_GetObjectItem(json, "scanArpEnabled");
    if (scanArpEnabled) 
    {
        settings.scanArpEnabled = (scanArpEnabled->valueint != 0); 
        //ESP_LOGI(TAG, "Updated scanArpEnabled: %d", settings.scanArpEnabled);
    }
    cJSON *scanArpInterval = cJSON_GetObjectItem(json, "scanArpInterval");
    if (scanArpInterval) {
    settings.scanArpInterval = scanArpInterval->valueint;
    ESP_LOGI(TAG, "Updated scanArpInterval: %d", settings.scanArpInterval);
    }
    cJSON *honeypotEnabled = cJSON_GetObjectItem(json, "honeypotEnabled");
    if (honeypotEnabled) settings.honeypotEnabled = honeypotEnabled->valueint;
    cJSON *honeypotIP = cJSON_GetObjectItem(json, "honeypotIP");
    if (honeypotIP && cJSON_IsString(honeypotIP)) 
    {
        strncpy(settings.honeypotIP, honeypotIP->valuestring, sizeof(settings.honeypotIP));
        //ESP_LOGI(TAG, "honeypotIP: %s", settings.honeypotIP);
    }
    cJSON *detectArpEnabled = cJSON_GetObjectItem(json, "detectArpEnabled");
    if (detectArpEnabled) settings.detectArpEnabled = detectArpEnabled->valueint;
    cJSON *detectArpInterval = cJSON_GetObjectItem(json, "detectArpInterval");
    if (detectArpInterval) 
    {
        settings.detectArpInterval = detectArpInterval->valueint;
        //ESP_LOGI(TAG, "detectArpInterval: %d", settings.detectArpInterval);
    }
    cJSON *differentIps = cJSON_GetObjectItem(json, "differentIps");
    if (differentIps && cJSON_IsNumber(differentIps)) 
    {
        settings.differentIps = differentIps->valueint;
        //ESP_LOGI(TAG, "differentIps: %d", settings.differentIps);
    }
    /* <<<<<<<<<<<<<<<<<<<<<<<<< Trang Account Setting >>>>>>>>>>>>>>>>>>>>>>>>>  */
    cJSON *userAdmin = cJSON_GetObjectItem(json, "userAdmin");
    if (userAdmin && cJSON_IsString(userAdmin)) 
    {
        strncpy(settings.userAdmin, userAdmin->valuestring, sizeof(settings.userAdmin));
        //ESP_LOGI(TAG, "userAdmin: %s", settings.userAdmin);
    }
    cJSON *passAdmin = cJSON_GetObjectItem(json, "passAdmin");
    if (passAdmin && cJSON_IsString(passAdmin)) 
    {
        strncpy(settings.passAdmin, passAdmin->valuestring, sizeof(settings.passAdmin));
        //ESP_LOGI(TAG, "passAdmin: %s", settings.passAdmin);
    }
    cJSON *userConsultant = cJSON_GetObjectItem(json, "userConsultant");
    if (userConsultant && cJSON_IsString(userConsultant)) strncpy(settings.userConsultant, userConsultant->valuestring, sizeof(settings.userConsultant));
    cJSON *passConsultant = cJSON_GetObjectItem(json, "passConsultant");
    if (passConsultant && cJSON_IsString(passConsultant)) strncpy(settings.passConsultant, passConsultant->valuestring, sizeof(settings.passConsultant));
    cJSON *codeGuest = cJSON_GetObjectItem(json, "codeGuest");
    if (codeGuest && cJSON_IsString(codeGuest)) strncpy(settings.codeGuest, codeGuest->valuestring, sizeof(settings.codeGuest));

    /* <<<<<<<<<<<<<<<<<<<<<<<<< Trang Alert Setting >>>>>>>>>>>>>>>>>>>>>>>>>  */
    //reset bảng:
    for (int i = 0; i < MAX_ROWS; i++) {
        memset(settings.rows[i].protocol, 0, sizeof(settings.rows[i].protocol));
        memset(settings.rows[i].IP1, 0, sizeof(settings.rows[i].IP1));
        memset(settings.rows[i].IP2, 0, sizeof(settings.rows[i].IP2));
        settings.rows[i].switchState = false;
    }
    cJSON *numRows = cJSON_GetObjectItem(json, "numRows");
    if (numRows) settings.numRows = numRows->valueint;
    char key[25];
    for(int i = 0; i < settings.numRows;i++)
    {
        
        snprintf(key, sizeof(key), "protocol%d", i+1);
        cJSON *protocol_item = cJSON_GetObjectItem(json, key);
        if (protocol_item){
            strncpy(settings.rows[i].protocol, protocol_item->valuestring, sizeof(settings.rows[i].protocol) - 1);
            settings.rows[i].protocol[sizeof(settings.rows[i].protocol) - 1] = '\0'; // Đảm bảo chuỗi kết thúc bằng ký tự NULL
            //ESP_LOGI(TAG, "protocol: %s", settings.rows[i].protocol);
        }
        snprintf(key, sizeof(key), "IP1%d", i+1);
        cJSON *IP1_item = cJSON_GetObjectItem(json, key);
        if (IP1_item){
            strncpy(settings.rows[i].IP1, IP1_item->valuestring, sizeof(settings.rows[i].IP1) - 1);
            settings.rows[i].IP1[sizeof(settings.rows[i].IP1) - 1] = '\0'; // Đảm bảo chuỗi kết thúc bằng ký tự NULL
            //ESP_LOGI(TAG, "IP1: %s", settings.rows[i].IP1);
        }
        snprintf(key, sizeof(key), "IP2%d", i+1);
        cJSON *IP2_item = cJSON_GetObjectItem(json, key);
        if (IP2_item){
            strncpy(settings.rows[i].IP2, IP2_item->valuestring, sizeof(settings.rows[i].IP2) - 1);
            settings.rows[i].IP2[sizeof(settings.rows[i].IP2) - 1] = '\0'; // Đảm bảo chuỗi kết thúc bằng ký tự NULL
            //ESP_LOGI(TAG, "IP2: %s", settings.rows[i].IP2);
        }
        snprintf(key, sizeof(key), "switchState%d", i+1);
        cJSON *switchState_item = cJSON_GetObjectItem(json, key);
        if (switchState_item){
            settings.rows[i].switchState = switchState_item->valueint;
            
            //ESP_LOGI(TAG, "switchState: %d", settings.rows[i].switchState);
        }
    }
        cJSON *phone1 = cJSON_GetObjectItem(json, "phone1");
        if (phone1) strncpy(settings.phone1, phone1->valuestring, sizeof(settings.phone1));
        cJSON *phone2 = cJSON_GetObjectItem(json, "phone2");
        if (phone2) strncpy(settings.phone2, phone2->valuestring, sizeof(settings.phone2));

    update_setting();
}



//ham gui cac cai dat dang json khi client yeu cau
void send_json2server(cJSON *json)
{
    cJSON_AddBoolToObject(json, "scanArpEnabled", settings.scanArpEnabled);
    cJSON_AddNumberToObject(json, "scanArpInterval", settings.scanArpInterval);
    cJSON_AddBoolToObject(json, "honeypotEnabled", settings.honeypotEnabled);
    cJSON_AddStringToObject(json, "honeypotIP", settings.honeypotIP);
    cJSON_AddBoolToObject(json, "detectArpEnabled", settings.detectArpEnabled);
    cJSON_AddNumberToObject(json, "detectArpInterval", settings.detectArpInterval);
    cJSON_AddNumberToObject(json, "differentIps", settings.differentIps);
    cJSON_AddStringToObject(json, "userAdmin", settings.userAdmin);
    cJSON_AddStringToObject(json, "passAdmin", settings.passAdmin);
    cJSON_AddStringToObject(json, "userConsultant", settings.userConsultant);
    cJSON_AddStringToObject(json, "passConsultant", settings.passConsultant);
    cJSON_AddStringToObject(json, "codeGuest", settings.codeGuest);
    cJSON_AddNumberToObject(json, "numRows", settings.numRows);
    cJSON_AddStringToObject(json, "phone1", settings.phone1);
    cJSON_AddStringToObject(json, "phone2", settings.phone2);
    // Tạo mảng JSON cho các hàng (rows)
    cJSON *rowsArray = cJSON_CreateArray();

    // Lặp qua từng hàng và thêm vào mảng JSON
    for (int i = 0; i < settings.numRows; i++) {
        // Tạo một đối tượng JSON cho mỗi hàng
        cJSON *rowObject = cJSON_CreateObject();
        
        // Thêm các trường vào đối tượng JSON của hàng
        cJSON_AddStringToObject(rowObject, "protocol", settings.rows[i].protocol);
        cJSON_AddStringToObject(rowObject, "IP1", settings.rows[i].IP1);
        cJSON_AddStringToObject(rowObject, "IP2", settings.rows[i].IP2);
        cJSON_AddBoolToObject(rowObject, "switchState", settings.rows[i].switchState);

        // Thêm hàng vào mảng rowsArray
        cJSON_AddItemToArray(rowsArray, rowObject);
    }
    cJSON_AddItemToObject(json, "rows", rowsArray);

}

void syslog_json2server(cJSON *json)
{
    // Tạo mảng JSON cho các hàng (rows)
    cJSON_AddNumberToObject(json, "numRows", log_count);
    cJSON *rowsArray = cJSON_CreateArray();

    char timelog[20];
    char message[250];
    
    for (int i = 0; i < log_count; i++) {
        cJSON *rowObject = cJSON_CreateObject();
        format_time(logs[i].timestamp, timelog, sizeof(timelog));
        // printf("%s %s\n", timelog, logs[i].event);
        snprintf(message, sizeof(message), "%s %s", timelog, logs[i].event);
        cJSON_AddStringToObject(rowObject, "log", message);
         cJSON_AddItemToArray(rowsArray, rowObject);
    }
    cJSON_AddItemToObject(json, "syslog", rowsArray);

}

// Hàm tự tạo timegm() cho các hệ thống không hỗ trợ
time_t my_timegm(struct tm *tm) {
    // Lưu trữ múi giờ hiện tại
    char *tz = getenv("TZ");
    
    // Đặt múi giờ tạm thời là UTC
    setenv("TZ", "UTC", 1);
    tzset();

    // Chuyển struct tm thành time_t theo UTC
    time_t t = mktime(tm);

    // Khôi phục múi giờ ban đầu
    if (tz) {
        setenv("TZ", tz, 1);
    } else {
        unsetenv("TZ");
    }
    tzset();

    return t;
}
// Hàm chuyển đổi time_t thành chuỗi ngày giờ dễ đọc
void format_time(time_t time, char *buffer, size_t size) {
    struct tm tm_info;
    
    // Chuyển đổi time_t sang struct tm theo múi giờ địa phương
    localtime_r(&time, &tm_info); 

    // Định dạng thành chuỗi
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", &tm_info);
}

// Khởi tạo thời gian ban đầu
void init_time() {
    initial_time = esp_timer_get_time()/1000000;
}
time_t offset = 0;
void log_event(const char *event) {
    if (log_count < MAX_LOGS) {
        logs[log_count].timestamp = esp_timer_get_time()/1000000 + offset;  // Thời gian trôi qua kể từ initial_time
        snprintf(logs[log_count].event, sizeof(logs[log_count].event), "%s", event);
        log_count++;
    } else {
        printf("Log storage is full.\n");
    }
}

void display_logs() {
    char timelog[20];
    for (int i = 0; i < log_count; i++) {
        format_time(logs[i].timestamp, timelog, sizeof(timelog));
        //printf("%s %s\n", timelog, logs[i].event);
    }
}

void sync_time_with_client(struct tm client_tm) {
    for (int i = 0; i < log_count; i++) {
        logs[i].timestamp -= offset;
    }
    time_t current_time = esp_timer_get_time()/1000000;    
    time_t client_time = mktime(&client_tm) + 25200;
    // Tính chênh lệch thời gian giữa client_time và current_time
    offset = client_time - current_time;
    printf("Gia tri offset time: %lld\n", offset);
    // Cập nhật lại initial_time theo client_time
    initial_time = client_time;

    // Điều chỉnh tất cả các log theo offset
    for (int i = 0; i < log_count; i++) {
        logs[i].timestamp += offset;
    }
}

void input_get_netif_callback(void* cb) //gọi từ ngoài main truyền hàm cb vào con trỏ hàm
{   
    get_netif_callback = cb; //truyền hàm vào con trỏ hàm
}

ip4_addr_t get_client_ip(int client_socket) {
    struct sockaddr_in6 client_addr;
    socklen_t addr_len = sizeof(client_addr);
    ip4_addr_t ipv4_addr;
    IP4_ADDR(&ipv4_addr, 0, 0, 0, 0);
    if (lwip_getpeername(client_socket, (struct sockaddr *)&client_addr, &addr_len) == 0) {
        char ip_str[INET6_ADDRSTRLEN];
        if (client_addr.sin6_family == AF_INET) {
            inet_ntoa_r(((struct sockaddr_in *)&client_addr)->sin_addr, ip_str, sizeof(ip_str));
        } else {
            inet6_ntoa_r(client_addr.sin6_addr, ip_str, sizeof(ip_str));
        }

        // Kiểm tra và loại bỏ "::FFFF:" nếu tồn tại trong chuỗi IP
        char *ipv4_mapped = strstr(ip_str, "::FFFF:");
        if (ipv4_mapped) {
            ip4addr_aton(ipv4_mapped + 7, &ipv4_addr);
            //printf("Client IPv4: %s\n", ipv4_mapped + 7);  // Bỏ qua "::FFFF:" và in phần IPv4
        } else {
            printf("Client IPv6: %s\n", ip_str);
        }
    } else {
        printf("Unable to get client IP\n");
    }
    return ipv4_addr;
}

void task_init_set(void)
{


    xTaskCreate(sendarp_task, "Arp Update Task", 2048, NULL, 6, &xHandle_sendarp);
    
    xTaskCreate(check_status_task, "Check and block Task", 2048, NULL, 4, &xHandle_check_status);

    xTaskCreate(check_arp_scan_task, "Check scan Task", 2048, NULL, 5, &xHandle_check_arp_scan);

    xTaskCreate(keepConnected_task, "Kepp connected Task", 4096, NULL, 4, &xHandle_keep_connect);
    http_set_callback_setting(setting_data_callback);


}