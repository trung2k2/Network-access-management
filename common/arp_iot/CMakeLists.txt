set(app_src arp_iot.c)
set(pri_req lwip main)
idf_component_register(SRCS ${app_src}
                    INCLUDE_DIRS "."
                    REQUIRES ${pri_req} driver esp_netif esp_eth 
                    PRIV_INCLUDE_DIRS "D:/Espressif/frameworks/esp-idf-v5.2.2/components/lwip/lwip/src/include/lwip")
