idf_component_register(SRCS "app_wifi.c"
                    INCLUDE_DIRS "."
                    PRIV_INCLUDE_DIRS "../../main/"
                    PRIV_REQUIRES main
                    REQUIRES wifi_provisioning esp_rainmaker qrcode)
