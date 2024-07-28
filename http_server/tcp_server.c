/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"

#include "wifi.h"
#include "http_server.h"
#include "tcp_server.h"

#define TCP_PORT 4242
#define DEBUG_printf printf
#define BUF_SIZE 2048
#define TEST_ITERATIONS 10
#define POLL_TIME_S 5

typedef struct TCP_SERVER_T_ {
    struct tcp_pcb *server_pcb;
    struct tcp_pcb *client_pcb;
} TCP_SERVER_T;

typedef struct TCP_CONNECT_STATE_T_ {
    struct tcp_pcb *pcb;
    size_t written_len;
    size_t sent_len;
    bool dont_close_on_send_finish;
} TCP_CONNECT_STATE_T;

on_tcp_data_recived tcp_server_on_data_recived = NULL;

bool tcp_server_debug_prints = false;

TCP_SERVER_T *tcp_server_state = NULL;

const char* last_wifi_ssid = NULL;

unsigned int tcp_server_max_read_size = 1024;

static TCP_SERVER_T* tcp_server_init(void) {
    TCP_SERVER_T *state = calloc(1, sizeof(TCP_SERVER_T));
    if (!state) {
        DEBUG_printf("failed to allocate state\n");
        return NULL;
    }
    return state;
}

static err_t tcp_close_client_connection(TCP_CONNECT_STATE_T *con_state, struct tcp_pcb *client_pcb, err_t close_err) {
    if (client_pcb) {
        assert(con_state && con_state->pcb == client_pcb);
        tcp_arg(client_pcb, NULL);
        tcp_poll(client_pcb, NULL, 0);
        tcp_sent(client_pcb, NULL);
        tcp_recv(client_pcb, NULL);
        tcp_err(client_pcb, NULL);
        err_t err = tcp_close(client_pcb);
        if (err != ERR_OK) {
            DEBUG_printf("close failed %d, calling abort\n", err);
            tcp_abort(client_pcb);
            close_err = ERR_ABRT;
        }
        if (con_state) {
            free(con_state);
        }
    }
    return close_err;
}

static void tcp_server_close(TCP_SERVER_T *state) {
    if (state->server_pcb) {
        tcp_arg(state->server_pcb, NULL);
        tcp_close(state->server_pcb);
        state->server_pcb = NULL;
    }
}

static err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    (void)tpcb;
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg;
    DEBUG_printf("tcp_server_sent %u\n", len);
    con_state->sent_len += len;

    if(con_state->sent_len >= con_state->written_len) {
        if(con_state->dont_close_on_send_finish) {
            if(tcp_server_debug_prints) { DEBUG_printf("all done\n"); }
            return tcp_close_client_connection(con_state, con_state->pcb, ERR_OK);
        }
    }

    return ERR_OK;
}

err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg;
    if (!p) {
        if(tcp_server_debug_prints) { DEBUG_printf("connection closed\n"); }
        return tcp_close_client_connection(con_state, tpcb, ERR_OK);
    }
    assert(con_state && con_state->pcb == pcb);
    if (p->tot_len > 0) {
        if(tcp_server_debug_prints) { DEBUG_printf("tcp_server_recv %d err %d\n", p->tot_len, err); }
#if 0
        for (struct pbuf *q = p; q != NULL; q = q->next) {
            DEBUG_printf("in: %.*s\n", q->len, q->payload);
        }
#endif
        // Copy the data into the buffer
        char read_data[tcp_server_max_read_size];
        unsigned short amount_read = pbuf_copy_partial(p, read_data, tcp_server_max_read_size-1, 0);

        if(tcp_server_on_data_recived != NULL) {
            TCP_CONNECTION_T* connection = (TCP_CONNECTION_T*)con_state;
            int error = tcp_server_on_data_recived(read_data, amount_read, connection);
            err_t err = (err_t)error;
            if (err != ERR_OK) {
                DEBUG_printf("failed to write tcp data %d\n", err);
                return tcp_close_client_connection(con_state, con_state->pcb, err);
            }
        } else {
            DEBUG_printf("tcp_server ERROR - No data recived handler set\n");
        }
        
        tcp_recved(tpcb, p->tot_len);
    }
    pbuf_free(p);
    return ERR_OK;
}

static err_t tcp_server_poll_event(void *arg, struct tcp_pcb *tpcb) {
    (void)tpcb;
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg;
    DEBUG_printf("tcp_server_poll_event_fn\n");
    return tcp_close_client_connection(con_state, con_state->pcb, ERR_OK); // Just disconnect clent?
}

static void tcp_server_err(void *arg, err_t err) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg;
    if (err != ERR_ABRT) {
        DEBUG_printf("tcp_client_err_fn %d\n", err);
        tcp_close_client_connection(con_state, con_state->pcb, err); // Just disconnect clent?
    }
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    (void)state;
    if (err != ERR_OK || client_pcb == NULL) {
        DEBUG_printf("Failure in accept\n");        
        return ERR_VAL;
    }
    DEBUG_printf("Client connected\n");

    // Create the state for the connection
    TCP_CONNECT_STATE_T *con_state = calloc(1, sizeof(TCP_CONNECT_STATE_T));
    if (!con_state) {
        DEBUG_printf("failed to allocate connect state\n");
        return ERR_MEM;
    }
    con_state->pcb = client_pcb; // for checking

    // setup connection to client
    tcp_arg(client_pcb, con_state);
    tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_poll(client_pcb, tcp_server_poll_event, POLL_TIME_S * 2);
    tcp_err(client_pcb, tcp_server_err);    

    return ERR_OK;
}

static bool tcp_server_open(void *arg, u16_t host_port) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    DEBUG_printf("Starting server at %s on port %u\n", ip4addr_ntoa(netif_ip4_addr(netif_list)), host_port);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        DEBUG_printf("failed to create pcb\n");
        return false;
    }

    err_t err = tcp_bind(pcb, NULL, host_port);
    if (err) {
        DEBUG_printf("failed to bind to port %u\n", host_port);
        return false;
    }

    state->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!state->server_pcb) {
        DEBUG_printf("failed to listen\n");
        if (pcb) {
            tcp_close(pcb);
        }
        return false;
    }

    tcp_arg(state->server_pcb, state);
    tcp_accept(state->server_pcb, tcp_server_accept);

    return true;
}

void tcp_server_poll(void)
{
    #if PICO_CYW43_ARCH_POLL
        // if you are using pico_cyw43_arch_poll, then you must poll periodically from your
        // main loop (not from a timer) to check for Wi-Fi driver or lwIP work that needs to be done.
        cyw43_arch_poll();
    #endif
}

int tcp_server_send_data(TCP_CONNECTION_T* connection, const char* data, const unsigned int data_len)
{
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)connection;
    struct tcp_pcb *tpcb = con_state->pcb;

    cyw43_arch_lwip_begin();
    con_state->written_len += data_len;
    err_t err = tcp_write(tpcb, data, data_len, TCP_WRITE_FLAG_COPY);    
    // if (err != ERR_OK) {
    //     DEBUG_printf("Failed to write data %d\n", err);
    //     return false;
    // }
    // return true;
    cyw43_arch_lwip_end();
    return err;
}

bool tcp_server_is_running()
{
    return tcp_server_state != NULL;
}

int tcp_server_start_timeout(const char* wifi_ssid, const char* wifi_password, uint32_t wifi_connect_timeout_ms, const char* hostname, u16_t host_port)
{
    wifi_debug_prints = tcp_server_debug_prints;
    int wifi_start_status = wifi_start_timeout(wifi_ssid, wifi_password, hostname, wifi_connect_timeout_ms);

    // check if wifi connected sucessfully
    if(wifi_start_status != WIFI_STATUS_SUCESS)
    {
        return wifi_start_status;
    }

    tcp_server_state = tcp_server_init();
    if (!tcp_server_state) {
        DEBUG_printf("failed to allocate tcp_server_state\n");
        return 3;
    }

    bool tcp_server_open_sucess = tcp_server_open(tcp_server_state, host_port);
    if (!tcp_server_open_sucess) {
        DEBUG_printf("failed to open tcp server\n");
        return 4;
    }

    last_wifi_ssid = wifi_ssid;

    return 0;
}
int tcp_server_start(const char* wifi_ssid, const char* wifi_password, const char* hostname, u16_t host_port)
{
    return tcp_server_start_timeout(wifi_ssid, wifi_password, TCP_SERVER_DEFAULT_WIFI_CONNECT_TIMEOUT_MS, hostname, host_port);
}

int tcp_server_stop(void)
{
    if(tcp_server_debug_prints) {
        DEBUG_printf("TCP Server disconnecting from wifi network: \"%s\"\n", last_wifi_ssid);
    }

    if(tcp_server_is_running())
    {
        tcp_server_close(tcp_server_state);
        free(tcp_server_state);
    }
    return wifi_stop();
}
