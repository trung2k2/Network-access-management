set(app_src blink_iot.c)
set(pri_req lwip main)
idf_component_register(SRCS ${app_src}
                    INCLUDE_DIRS "."
                    REQUIRES driver)
