#ifndef PICOW_TCP_SERVER_H
#define PICOW_TCP_SERVER_H

#define TCP_SERVER_DEFAULT_WIFI_CONNECT_TIMEOUT_MS 30000

// dont write more then this_value-1 in your generator function, if you make this bigger it will change the internal buffer size, which will work up to an extent, then break because of pico memory limitations.
#define MAX_DATA_SIZE 4096
#define MAX_HEADER_SIZE 128

typedef struct TCP_CONNECTION_T_ {
    int dummy_data;
} TCP_CONNECTION_T;

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

typedef int (*on_tcp_data_recived)(const char* data, const unsigned int data_len, TCP_CONNECTION_T* connection);

extern unsigned int tcp_server_max_read_size;
extern bool tcp_server_debug_prints;
extern on_tcp_data_recived tcp_server_on_data_recived;


bool tcp_server_send_data(TCP_CONNECTION_T* connection, const char* data, const unsigned int data_len);

bool tcp_server_is_running();

int tcp_server_start_timeout(const char* wifi_ssid, const char* wifi_password, uint32_t wifi_connect_timeout_ms, const char* hostname);
int tcp_server_start(const char* wifi_ssid, const char* wifi_password, const char* hostname);
int tcp_server_stop(void);


#ifdef __cplusplus
};
#endif

#endif // PICOW_TCP_SERVER_H