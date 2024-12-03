set(app_src http_server_iot.c)
set(pri_req main)
idf_component_register(SRCS ${app_src}
                    INCLUDE_DIRS "."
                    REQUIRES ${pri_req} esp_wifi esp_http_server esp-tls)
#include_directories("D:/Espressif/frameworks/esp-idf-v5.2.2/components/esp-tls")