#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "wifi.h"

bool wifi_debug_prints = false;
bool wifi_connected = false;

void wifi_set_host_name(const char* hostname) {
    cyw43_arch_lwip_begin();
    struct netif *n = &cyw43_state.netif[CYW43_ITF_STA];
    netif_set_hostname(n, hostname);
    netif_set_up(n);
    cyw43_arch_lwip_end();
}

int wifi_start(const char* wifi_ssid, const char* wifi_password, const char* hostname)
{
    return wifi_start_timeout(wifi_ssid, wifi_password, hostname, WIFI_CONNECT_DEFAULT_TIMEOUT_MS);
}

int wifi_start_timeout(const char* wifi_ssid, const char* wifi_password, const char* hostname, uint32_t wifi_connect_timeout_ms)
{
    if(wifi_connected) {
        wifi_stop();
    }

    if (cyw43_arch_init()) {
        printf("failed to initialise cyw43_arch_init\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();

    wifi_set_host_name(hostname);
    if(wifi_debug_prints) {
        printf("Wi-Fi Hostname set to: \"%s\"\n", hostname);
    }

    if(wifi_debug_prints) {
        printf("TCP Server connecting to Wi-Fi: \"%s\"\n", wifi_ssid);
    }
    if (cyw43_arch_wifi_connect_timeout_ms(wifi_ssid, wifi_password, CYW43_AUTH_WPA2_AES_PSK, wifi_connect_timeout_ms)) {
        if(wifi_debug_prints) {
            printf("TCP Server failed to connect to wifi network \"%s\"\n", wifi_ssid);
        }
        return 2;
    } else {
        if(wifi_debug_prints) {
            printf("TCP Server Wi-Fi Connected: \"%s\"\n", wifi_ssid);
        }
    }

    wifi_connected = true;
    return WIFI_STATUS_SUCESS;
}

int wifi_stop(void)
{
    wifi_connected = false;
    cyw43_arch_deinit();
    return WIFI_STATUS_SUCESS;
}