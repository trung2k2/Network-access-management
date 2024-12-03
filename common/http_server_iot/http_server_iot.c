

/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


//#if !CONFIG_IDF_TARGET_LINUX
#include <esp_wifi.h>
#include "esp_event.h"
#include <esp_log.h>
#include <esp_system.h>
#include <sys/param.h>
#include "http_server_iot.h"
#include <esp_http_server.h>
#include "esp_netif.h"
#include "esp_tls.h"
#include "lwip/etharp.h"
#include "cJSON.h"

//#endif  // !CONFIG_IDF_TARGET_LINUX

#define EXAMPLE_HTTP_QUERY_KEY_MAX_LEN  (64)
static httpd_handle_t server = NULL;
static http_post_callback_t http_post_switch_callback = NULL;
static http_get_callback_t http_post_setting_callback = NULL;

char correct_username[16] = "admin";
char correct_password[16] = "123456";
char correct_codeguest[9] = "";
char correct_username_consultant[16] = "consultant";
char correct_password_consultant[16] = "123456";
int check_sign_in = 0;

/* A simple example that demonstrates how to create GET and POST
 * handlers for the web server.
 */

static const char *TAG = "SERVER";

extern const uint8_t anh1_start[] asm("_binary_anhmau_jpeg_start");
extern const uint8_t anh1_end[] asm("_binary_anhmau_jpeg_end");
extern const uint8_t anh2_start[] asm("_binary_anhmau2_jpeg_start");
extern const uint8_t anh2_end[] asm("_binary_anhmau2_jpeg_end");
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
extern const uint8_t index2_html_start[] asm("_binary_index2_html_start");
extern const uint8_t index2_html_end[] asm("_binary_index2_html_end");
extern const uint8_t index3_html_start[] asm("_binary_index3_html_start");
extern const uint8_t index3_html_end[] asm("_binary_index3_html_end");
extern const uint8_t index4_html_start[] asm("_binary_index4_html_start");
extern const uint8_t index4_html_end[] asm("_binary_index4_html_end");
extern const uint8_t index5_html_start[] asm("_binary_index5_html_start");
extern const uint8_t index5_html_end[] asm("_binary_index5_html_end");
extern const uint8_t all_css_start[] asm("_binary_all_css_start");
extern const uint8_t all_css_end[] asm("_binary_all_css_end");
extern const uint8_t jquery_3_6_0_min_js_start[] asm("_binary_jquery_3_6_0_min_js_start");
extern const uint8_t jquery_3_6_0_min_js_end[] asm("_binary_jquery_3_6_0_min_js_end");
extern const uint8_t jspdf_plugin_autotable_min_js_start[] asm("_binary_jspdf_plugin_autotable_min_js_start");
extern const uint8_t jspdf_plugin_autotable_min_js_end[] asm("_binary_jspdf_plugin_autotable_min_js_end");
extern const uint8_t jspdf_umd_min_js_start[] asm("_binary_jspdf_umd_min_js_start");
extern const uint8_t jspdf_umd_min_js_end[] asm("_binary_jspdf_umd_min_js_end");

image_info_t images[] = {
    {"anh1", anh1_start, anh1_end},
    {"anh2", anh2_start, anh2_end},
    // Thêm các ảnh khác tại đây
};

bool is_logged_in = false; //bien kiem tra trang thai login cua client
bool is_logged_in_consultant = false;
static bool check_login(httpd_req_t *req) { //kiem tra dang nhap cookie
    char session_token[32] = {0};
    size_t token_len = httpd_req_get_hdr_value_len(req, "Cookie") + 1;

    // Kiểm tra nếu có cookie hay không
    if (token_len > 1) {
        httpd_req_get_hdr_value_str(req, "Cookie", session_token, token_len);
    
    // Kiểm tra xem cookie có chứa session_token không
    if (strstr(session_token, "session_token=123456") != NULL) {
        int client_socket = httpd_req_to_sockfd(req);  // Lấy socket descriptor của client
        ip4_addr_t client_ip = get_client_ip(client_socket);
        if(!ip4_addr_cmp(&zero_ip, &client_ip) && !check_sign_in)
        {
            trust_sign(client_ip, "admin");
            check_sign_in = 1;
        }
        return true; // Đã đăng nhập
    }
    }
    return false; // Chưa đăng nhập
}

static bool check_login_consultant(httpd_req_t *req) { //kiem tra dang nhap cookie
    char session_token[32] = {0};
    size_t token_len = httpd_req_get_hdr_value_len(req, "Cookie") + 1;

    // Kiểm tra nếu có cookie hay không
    if (token_len > 1) {
        httpd_req_get_hdr_value_str(req, "Cookie", session_token, token_len);
    
    // Kiểm tra xem cookie có chứa session_token không
    if (strstr(session_token, "session_token=654321") != NULL) {
        int client_socket = httpd_req_to_sockfd(req);  // Lấy socket descriptor của client
        ip4_addr_t client_ip = get_client_ip(client_socket);
        if(!ip4_addr_cmp(&zero_ip, &client_ip))
        {
            trust_sign(client_ip, "consultant");
        }
        
        return true; // Đã đăng nhập
    }
    }
    return false; // Chưa đăng nhập
}

/* An HTTP POST handler */


static esp_err_t sign_in_get_handler(httpd_req_t *req)
{
    //const char* resp_str = (const char*) "Send response request";
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index2_html_start, index2_html_end - index2_html_start); //send lại phản hồi khi client request vào dht11
    return ESP_OK;
}

static const httpd_uri_t get_sign_in= {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = sign_in_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

static esp_err_t sw1_post_handler(httpd_req_t *req)
{
    char buf[100];
        /* Send back the same data */
    httpd_req_recv(req, buf, req->content_len);
    http_post_switch_callback(buf, req->content_len);
    printf("DATA: %s\n", buf);
    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t sw1_post_data= {
    .uri       = "/switch1",
    .method    = HTTP_POST,
    .handler   = sw1_post_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};


static esp_err_t seting_post_handler(httpd_req_t *req)
{
    char buf[400];
        /* Send back the same data */
        
    httpd_req_recv(req, buf, req->content_len);
    cJSON *json = cJSON_Parse(buf);
    if (json != NULL)
    http_post_setting_callback(buf, req->content_len, json);
        // Trả về kết quả
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\": \"success\"}");
    cJSON_Delete(json);
    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}


static const httpd_uri_t seting_post_data= {
    .uri       = "/updateSetting",
    .method    = HTTP_POST,
    .handler   = seting_post_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

esp_err_t setting_data_handler(httpd_req_t *req) {
    cJSON *json = cJSON_CreateObject();
    send_json2server(json);
    const char *response = cJSON_Print(json);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));

    cJSON_Delete(json);
    free((void *)response); 
    return ESP_OK;
}
static const httpd_uri_t get_data_setting= {
    .uri       = "/getSetting",
    .method    = HTTP_GET,
    .handler   = setting_data_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

esp_err_t syslog_data_handler(httpd_req_t *req) {
    cJSON *json = cJSON_CreateObject();
    syslog_json2server(json);
    const char *response = cJSON_Print(json);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));

    cJSON_Delete(json);
    free((void *)response); 
    return ESP_OK;
}
static const httpd_uri_t get_data_syslog= {
    .uri       = "/getSyslog",
    .method    = HTTP_GET,
    .handler   = syslog_data_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};



static esp_err_t sign_post_handler(httpd_req_t *req) //xử lý đăng nhập
{
    int login_successful = 0;
    int login_successful_consultant = 0;
    int login_successful_guest = 0;
    char buf[100];
    int ret, remaining = req->content_len;

    // Nhận dữ liệu POST
    if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) < 0) {
        // Xử lý lỗi khi nhận dữ liệu
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);  // Trả về lỗi 408 nếu timeout
        }
        return ESP_FAIL;
    }
    buf[ret] = '\0'; // Kết thúc chuỗi dữ liệu

    //printf("DATA: %s\n", buf);  // In ra dữ liệu đã nhận

    // Kiểm tra nếu yêu cầu là đăng xuất
    if (strstr(buf, "action=logout") != NULL) {
        // Xóa cookie bằng cách thiết lập thời gian hết hạn trong quá khứ
        httpd_resp_set_hdr(req, "Set-Cookie", "session_token=0; Path=/; HttpOnly");

        // Trả về phản hồi thành công
        const char* resp = "{\"success\": true}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp, strlen(resp));
        return ESP_OK;
    }

    // Kiểm tra thông tin đăng nhập từ dữ liệu POST
    

    // Giả sử dữ liệu được gửi là dạng `username=admin&password=123456`
    char *username = strstr(buf, "username=");
    char *password = strstr(buf, "password=");
    char *codeguest = strstr(buf, "code=");
    if (username && password) {
        username += 9; // Bỏ qua "username=" để lấy giá trị
        password += 9; // Bỏ qua "password=" để lấy giá trị

        // Xử lý username và password từ `buf` "user=alice&password=1234"
        char *username_end = strchr(username, '&');
        if (username_end) *username_end = '\0'; // Kết thúc chuỗi username

        // Kiểm tra nếu username và password khớp
        if (strcmp(username, correct_username) == 0 && strcmp(password, correct_password) == 0) {
            login_successful = 1;
        }
        if (strcmp(username, correct_username_consultant) == 0 && strcmp(password, correct_password_consultant) == 0) {
            login_successful_consultant = 1;
        }
    }

    if (codeguest) {
        codeguest += 5; // Bỏ qua "code=" để lấy giá trị
        // Kiểm tra nếu username và password khớp
        if (strcmp(codeguest, correct_codeguest) == 0) {
            login_successful_guest = 1;
        }
    }

    if (login_successful) {
        // Gửi token trong cookie
        httpd_resp_set_hdr(req, "Set-Cookie", "session_token=123456; Path=/; HttpOnly");

        // Trả về phản hồi đăng nhập thành công
        const char *resp = "{\"success\": true, \"role\": \"admin\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp, strlen(resp));

        // Đánh dấu kết thúc
        httpd_resp_send_chunk(req, NULL, 0);
    } 
    else if (login_successful_consultant) {
        // Gửi token trong cookie
        httpd_resp_set_hdr(req, "Set-Cookie", "session_token=654321; Path=/; HttpOnly");

        // Trả về phản hồi đăng nhập thành công
        const char *resp = "{\"success\": true, \"role\": \"consultant\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp, strlen(resp));

        // Đánh dấu kết thúc
        httpd_resp_send_chunk(req, NULL, 0);
    } 
    if (login_successful_guest) {
        int client_socket = httpd_req_to_sockfd(req);  // Lấy socket descriptor của client
        ip4_addr_t client_ip = get_client_ip(client_socket);
        if(!ip4_addr_cmp(&zero_ip, &client_ip))
        {
            trust_sign(client_ip, "guest");
        }
        // Trả về phản hồi đăng nhập thành công
        const char *resp = "{\"success\": true, \"role\": \"guest\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp, strlen(resp));

        // Đánh dấu kết thúc
        httpd_resp_send_chunk(req, NULL, 0);
    } 
    else {
        // Trả về phản hồi đăng nhập thất bại
        const char *resp = "{\"success\": false}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp, strlen(resp));

        // Đánh dấu kết thúc
        httpd_resp_send_chunk(req, NULL, 0);
    }

    return ESP_OK;
}


static const httpd_uri_t sign_data= {
    .uri       = "/login",
    .method    = HTTP_POST,
    .handler   = sign_post_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

/* An HTTP GET handler */
static esp_err_t hello_get_handler(httpd_req_t *req) //trang chủ
{
    if (!check_login(req)) {
        // Nếu chưa đăng nhập, trả về mã lỗi hoặc chuyển hướng
        httpd_resp_set_status(req, "302 Found"); // Mã trạng thái 302
        httpd_resp_set_hdr(req, "Location", "/"); // Đặt tiêu đề Location
        httpd_resp_send(req, NULL, 0); // Gửi phản hồi rỗng
        return ESP_OK; // Kết thúc xử lý
    }
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start); //send lại phản hồi khi client request vào dht11
    return ESP_OK;
}

static const httpd_uri_t get_dht11= {
    .uri       = "/trangchu",
    .method    = HTTP_GET,
    .handler   = hello_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

static esp_err_t syslog_get_handler(httpd_req_t *req) //trang chủ
{
    if (!check_login(req)) {
        // Nếu chưa đăng nhập, trả về mã lỗi hoặc chuyển hướng
        httpd_resp_set_status(req, "302 Found"); // Mã trạng thái 302
        httpd_resp_set_hdr(req, "Location", "/"); // Đặt tiêu đề Location
        httpd_resp_send(req, NULL, 0); // Gửi phản hồi rỗng
        return ESP_OK; // Kết thúc xử lý
    }
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index4_html_start, index4_html_end - index4_html_start); //send lại phản hồi khi client request vào dht11
    return ESP_OK;
}

static const httpd_uri_t get_syslog= {
    .uri       = "/nhatky",
    .method    = HTTP_GET,
    .handler   = syslog_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

static esp_err_t consultant_get_handler(httpd_req_t *req) //trang chủ
{
    if (!check_login_consultant(req)) {
        // Nếu chưa đăng nhập, trả về mã lỗi hoặc chuyển hướng
        httpd_resp_set_status(req, "302 Found"); // Mã trạng thái 302
        httpd_resp_set_hdr(req, "Location", "/"); // Đặt tiêu đề Location
        httpd_resp_send(req, NULL, 0); // Gửi phản hồi rỗng
        return ESP_OK; // Kết thúc xử lý
    }
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index5_html_start, index5_html_end - index5_html_start); //send lại phản hồi khi client request vào dht11
    return ESP_OK;
}

static const httpd_uri_t get_consultant= {
    .uri       = "/trangchuconsultant",
    .method    = HTTP_GET,
    .handler   = consultant_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

static esp_err_t setting_get_handler(httpd_req_t *req)
{
    if (!check_login(req)) {
        // Nếu chưa đăng nhập, trả về mã lỗi hoặc chuyển hướng
        httpd_resp_set_status(req, "302 Found"); // Mã trạng thái 302
        httpd_resp_set_hdr(req, "Location", "/"); // Đặt tiêu đề Location
        httpd_resp_send(req, NULL, 0); // Gửi phản hồi rỗng
        return ESP_OK; // Kết thúc xử lý
    }
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index3_html_start, index3_html_end - index3_html_start); //send lại phản hồi khi client request vào dht11
    return ESP_OK;
}

static const httpd_uri_t get_setting= {
    .uri       = "/caidat",
    .method    = HTTP_GET,
    .handler   = setting_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};
esp_err_t image_get_handler(httpd_req_t *req) {
    if (strncmp(req->uri, "/anh1", 5) == 0) {
        // Trả về ảnh 1
        const uint8_t* start = (const uint8_t*) anh1_start;
            const uint8_t* end = (const uint8_t*) anh1_end;
        httpd_resp_set_type(req, "image/jpeg");
        httpd_resp_send(req, (const char *)start, end - start);
        return ESP_OK;
    } else if (strncmp(req->uri, "/anh2", 5) == 0) {
        // Trả về ảnh 2
        const uint8_t* start = (const uint8_t*) anh2_start;
        const uint8_t* end = (const uint8_t*) anh2_end;
        httpd_resp_set_type(req, "image/jpeg");
        httpd_resp_send(req, (const char *)start, end - start);
        return ESP_OK;
    }

    return ESP_OK;
}
static const httpd_uri_t get_image_anh1 = {
    .uri       = "/anh1",
    .method    = HTTP_GET,
    .handler   = image_get_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t get_image_anh2 = {
    .uri       = "/anh2",
    .method    = HTTP_GET,
    .handler   = image_get_handler,
    .user_ctx  = NULL
};

static esp_err_t css_get_handler(httpd_req_t *req)
{
    //const char* resp_str = (const char*) "Send response request";
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char *)all_css_start, all_css_end - all_css_start); //send lại phản hồi khi client request vào dht11
    return ESP_OK;
}

static const httpd_uri_t get_css= {
    .uri       = "/all_css",
    .method    = HTTP_GET,
    .handler   = css_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

static esp_err_t js360_get_handler(httpd_req_t *req)
{
    //const char* resp_str = (const char*) "Send response request";
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, (const char *)jquery_3_6_0_min_js_start, jquery_3_6_0_min_js_end - jquery_3_6_0_min_js_start); //send lại phản hồi khi client request vào dht11
    return ESP_OK;
}

static const httpd_uri_t get_js360= {
    .uri       = "/jquery-3.6.0.min",
    .method    = HTTP_GET,
    .handler   = js360_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

static esp_err_t jspdf_plugin_get_handler(httpd_req_t *req)
{
    //const char* resp_str = (const char*) "Send response request";
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, (const char *)jspdf_plugin_autotable_min_js_start, jspdf_plugin_autotable_min_js_end - jspdf_plugin_autotable_min_js_start); //send lại phản hồi khi client request vào dht11
    return ESP_OK;
}

static const httpd_uri_t get_jspdf_plugin= {
    .uri       = "/jspdf_plugin_autotable_min",
    .method    = HTTP_GET,
    .handler   = jspdf_plugin_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

static esp_err_t jspdf_umd_get_handler(httpd_req_t *req)
{
    //const char* resp_str = (const char*) "Send response request";
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, (const char *)jspdf_umd_min_js_start, jspdf_umd_min_js_end - jspdf_umd_min_js_start); //send lại phản hồi khi client request vào dht11
    return ESP_OK;
}

static const httpd_uri_t get_jspdf_umd= {
    .uri       = "/jspdf_umd_min",
    .method    = HTTP_GET,
    .handler   = jspdf_umd_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

// Hàm xử lý POST request để nhận thời gian từ client
esp_err_t set_time_handler(httpd_req_t *req) {
    // Khởi tạo buffer để nhận dữ liệu

    char content[100];
    int ret = httpd_req_recv(req, content, sizeof(content));
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);  // Gửi phản hồi timeout
        }
        return ESP_FAIL;
    }

    // Parse dữ liệu JSON từ client
    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        return ESP_FAIL;
    }
    struct tm tm;
    // Đặt múi giờ thành UTC
    tm.tm_isdst = 0;  // Không có DST
    cJSON *timeItem = cJSON_GetObjectItem(json, "time");
    if (cJSON_IsString(timeItem) && (timeItem->valuestring != NULL)) {
        char timeString[30];
        strncpy(timeString, timeItem->valuestring, sizeof(timeString) - 1);
        timeString[sizeof(timeString) - 1] = '\0';  // Đảm bảo chuỗi kết thúc bằng null

        memset(&tm, 0, sizeof(struct tm));

        char *zPos = strchr(timeString, 'Z');
        if (zPos) {
            *zPos = '\0';  // Tạm thời loại bỏ 'Z' để phân tích
        }

        // Bỏ qua phần giây và Z để phân tích thời gian theo ISO 8601 (YYYY-MM-DDTHH:MM:SS)
        if (strptime(timeString, "%Y-%m-%dT%H:%M:%S", &tm) == NULL) {
            printf("Failed to parse time string.\n");
        }
        sync_time_with_client(tm);
    }

    cJSON_Delete(json);  // Giải phóng bộ nhớ JSON

            // Trả về phản hồi đăng nhập thành công
        // Tạo chuỗi JSON chứa biến initial_time
        char resp[128];
        char formattedTime[20]; // Dùng để chứa chuỗi thời gian định dạng dễ đọc
        format_time(initial_time, formattedTime, sizeof(formattedTime));
        snprintf(resp, sizeof(resp), "{\"success\": true, \"initial_time\": %lld, \"formattedTime\": \"%s\"}", 
             (long long)initial_time, formattedTime);
             printf("initial_time_after_syn: %s.\n", formattedTime);

        // Đặt kiểu dữ liệu phản hồi là JSON và gửi dữ liệu về client
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp, strlen(resp));

        // Đánh dấu kết thúc
        httpd_resp_send_chunk(req, NULL, 0);

    return ESP_OK;
}

// Cấu hình route cho POST /setTime
httpd_uri_t set_time = {
    .uri       = "/setTime",
    .method    = HTTP_POST,
    .handler   = set_time_handler,
    .user_ctx  = NULL
};



static esp_err_t arp_table_get_handler(httpd_req_t *req)  //cap nhat bang arp
{
    cJSON *json = cJSON_CreateObject();
    send_arpupdate(json);
    const char *response = cJSON_Print(json);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));

    cJSON_Delete(json);
    free((void *)response); 
    return ESP_OK;
}

static const httpd_uri_t get_data_arp= {
    .uri       = "/getdataarp",
    .method    = HTTP_GET,
    .handler   = arp_table_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};


esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    if (strcmp("/trangchu", req->uri) == 0) { //-> truy xuất vào struct vì hàm nhập vào khai báo *req là con trỏ, nếu không có * thì dùng . 
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/hello URI is not available");
        /* Return ESP_OK to keep underlying socket open */
        return ESP_OK;
    } 
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}

/* An HTTP PUT handler. This demonstrates realtime
 * registration and deregistration of URI handlers
 */


void start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
#if CONFIG_IDF_TARGET_LINUX
    // Setting port as 8001 when building for Linux. Port 80 can be used only by a priviliged user in linux.
    // So when a unpriviliged user tries to run the application, it throws bind error and the server is not started.
    // Port 8001 can be used by an unpriviliged user as well. So the application will not throw bind error and the
    // server will be started.
    config.server_port = 8001;
#endif // !CONFIG_IDF_TARGET_LINUX
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &sw1_post_data);     //xu ly switch
        httpd_register_uri_handler(server, &get_dht11);         //trang chu
        httpd_register_uri_handler(server, &get_sign_in);       //trang dang nhap
        httpd_register_uri_handler(server, &get_image_anh1);    //anh1
        httpd_register_uri_handler(server, &get_image_anh2);    //anh2
        httpd_register_uri_handler(server, &get_setting);       //trang cai dat
        httpd_register_uri_handler(server, &get_syslog);       //trang syslog
        httpd_register_uri_handler(server, &get_data_arp);      //lay du lieu bang arp
        httpd_register_uri_handler(server, &seting_post_data);  //cap nhat du lieu setting
        httpd_register_uri_handler(server, &sign_data);         //xu ly du lieu cai dat
        httpd_register_uri_handler(server, &set_time);         //set time
        httpd_register_uri_handler(server, &get_data_setting);  //lay du lieu setting
        httpd_register_uri_handler(server, &get_data_syslog);  //lay du lieu setting
        httpd_register_uri_handler(server, &get_consultant);  //trang chu consultaint
        httpd_register_uri_handler(server, &get_css);  
        httpd_register_uri_handler(server, &get_js360);  
        httpd_register_uri_handler(server, &get_jspdf_plugin);  
        httpd_register_uri_handler(server, &get_jspdf_umd);  
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
    }
    else{
    ESP_LOGI(TAG, "Error starting server!");
    }
}


void stop_webserver(void)
{
    // Stop the httpd server
    httpd_stop(server);
}

void http_set_callback_switch(void *cb)
{
    http_post_switch_callback = cb;
}

void http_set_callback_setting(void *cb)
{
    http_post_setting_callback = cb;
}


