#ifndef PICOW_WIFI_H
#define PICOW_WIFI_H

#define WIFI_CONNECT_DEFAULT_TIMEOUT_MS 30000
#define WIFI_STATUS_SUCESS 0

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

extern bool wifi_debug_prints;
extern bool wifi_connected;

int wifi_start(const char* wifi_ssid, const char* wifi_password);
int wifi_start_timeout(const char* wifi_ssid, const char* wifi_password, uint32_t wifi_connect_timeout_ms);
int wifi_stop(void);

#ifdef __cplusplus
};
#endif

#endif // PICOW_WIFI_H