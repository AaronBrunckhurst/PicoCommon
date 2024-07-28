#ifndef PICOW_HTTP_SERVER_H
#define PICOW_HTTP_SERVER_H

#include "tcp_server.h"

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

#define MAX_HEADER_SIZE 128
#define MAX_DATA_SIZE 4096

// if you change this, you will change the default page that clients will be shown. MAKE SURE THIS EXISTS
#define DEFAULT_URL "/index.html"

typedef void (*html_page_generator_func_t)(const char *params, TCP_CONNECTION_T* connection, int *write_error_code);

typedef bool (*ap_get_handeler_func_t)(const char *request, const char *params, TCP_CONNECTION_T* connection, int *write_error_code);

// set this in your program to change what page the users are brought to by default
extern const char* default_url;

// set this if you need to handle get requests in a specific way
// and request endpoint already registered with register_html_generator will not get passed to this function
extern ap_get_handeler_func_t get_handler;

extern bool http_server_debug_prints;

int http_server_start_timeout(const char* wifi_ssid, const char* wifi_password, uint32_t wifi_connect_timeout_ms, const char* hostname);
int http_server_start(const char* wifi_ssid, const char* wifi_password, const char* hostname);
int http_server_stop(void);

#ifdef __cplusplus
};
#endif

#endif // PICOW_HTTP_SERVER_H