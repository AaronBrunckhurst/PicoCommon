/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include "dhcpserver.h"
#include "dnsserver.h"

#include "url_collection.h"

#include "picow_access_point.h"

#include "html_helper.c"

// dont write more then this_value-1 in your generator function, if you make this bigger it will change the internal buffer size, which will work up to an extent, then break because of pico memory limitations.
#define MAX_DATA_SIZE 4096
#define MAX_HEADER_SIZE 128

#define TCP_PORT 80
#define DEBUG_printf printf
#define POLL_TIME_S 5
#define HTTP_GET "GET"
#define HTTP_GET_CODE 200
#define HTTP_RESPONSE_HEADERS "HTTP/1.1 %d OK\nServer: PICOW (RP2040)\nContent-Length: %d\nContent-Type: text/html; charset=utf-8\nConnection: close\n\n"
#define HTTP_RESPONSE_REDIRECT_HEADER "HTTP/1.1 302 Redirect\nServer: PICOW (RP2040)\nLocation: http://%s%s\n\n"
#define HTTP_RESPONSE_NOT_FOUND_HEADER "HTTP/1.1 404 Not Found\nServer: PICOW (RP2040)\nContent-Length: %d\nContent-Type: text/html; charset=utf-8\nConnection: close\n\n"

#define NOT_FOUND_BODY "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html>\n<head>\n   <title>404 Not Found</title>\n</head>\n<body>\n   <h1>Not Found</h1>\n   <p>The requested URL \"%s\" was not found on this server.</p>\n</body>\n</html>"

#define DEFAULT_REQUEST_ONE "/connecttest.txt"
#define DEFAULT_REQUEST_TWO "/favicon.ico"
#define DEFAULT_REQUEST_THREE "/redirect"

typedef struct TCP_SERVER_T_ {
    struct tcp_pcb *server_pcb;
    bool complete;
    ip_addr_t gw;
    async_context_t *context;
} TCP_SERVER_T;

typedef struct TCP_CONNECT_STATE_T_ {
    struct tcp_pcb *pcb;
    size_t sent_len;
    char headers[MAX_HEADER_SIZE];
    char result[MAX_DATA_SIZE];
    size_t header_len;
    size_t result_len;
    ip_addr_t *gw;
} TCP_CONNECT_STATE_T;

bool debug_print = false;

// function prototypes
bool is_default_requests(const char* request, const unsigned max_size);
static int find_first_index(const char *str, char ch, unsigned int max_length);

const char* default_url = DEFAULT_URL;
ap_get_handeler_func_t get_handler = NULL;

// globals, for init and deinit
static TCP_SERVER_T* state;
static dhcp_server_t dhcp_server;
static dns_server_t dns_server;

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

static err_t ap_tcp_server_sent(void *arg, struct tcp_pcb *pcb, u16_t len) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg;
    if(debug_print) { DEBUG_printf("tcp_server_sent %u\n", len); }
    con_state->sent_len += len;
    if (con_state->sent_len >= con_state->header_len + con_state->result_len) {
        if(debug_print) { DEBUG_printf("all done\n"); }
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    }
    return ERR_OK;
}

int ap_send_get_responce(AP_TCP_CONNECTION_T* connection, const char* data, const unsigned int data_len)
{
    TCP_CONNECT_STATE_T* state = (TCP_CONNECT_STATE_T*)connection;

    char header_data[MAX_HEADER_SIZE] = {0};
    size_t header_len = snprintf(header_data, sizeof(header_data), HTTP_RESPONSE_HEADERS,
            HTTP_GET_CODE, data_len);
    if (header_len > sizeof(header_data) - 1) {
        DEBUG_printf("Too much header data %d\n", header_len);
        return tcp_close_client_connection(state, state->pcb, ERR_CLSD);
    }

    // Send the headers to the client
    err_t err = ap_tcp_write(connection, header_data, header_len);
    if (err != ERR_OK) { return err; }

    // Send the body to the client
    err = ap_tcp_write(connection, data, data_len);
    if (err != ERR_OK) { return err; }
    return ERR_OK;
}

static err_t http_get_generate(char *request, char *params, struct tcp_pcb *pcb, TCP_CONNECT_STATE_T *con_state)
{
    AP_TCP_CONNECTION_T* connection = (AP_TCP_CONNECTION_T*)con_state;

    if(debug_print) {
        DEBUG_printf("Request: %s?%s\n", request, params);
    }

    // Generate content    
    int send_error_code = 0;
    bool html_generated = create_html_page(request, params, connection, &send_error_code);
    if(html_generated)
    {
        return send_error_code;
    }

    if(get_handler != NULL)
    {
        bool handeled = get_handler(request, params, connection, &send_error_code);
        if(handeled)
        {
            return send_error_code;
        }
    }    

    // Check we had enough buffer space
    if (con_state->result_len > sizeof(con_state->result) - 1) {
        DEBUG_printf("Too much result data %d\n", con_state->result_len);
        return tcp_close_client_connection(con_state, pcb, ERR_CLSD);
    }

    // Generate web page
    if (con_state->result_len > 0) {
        con_state->header_len = snprintf(con_state->headers, sizeof(con_state->headers), HTTP_RESPONSE_HEADERS,
            HTTP_GET_CODE, con_state->result_len);
        if (con_state->header_len > sizeof(con_state->headers) - 1) {
            DEBUG_printf("Too much header data %d\n", con_state->header_len);
            return tcp_close_client_connection(con_state, pcb, ERR_CLSD);
        }
    } else {        
        if(is_default_requests(request, MAX_HEADER_SIZE))
        {
            // Send redirect
            con_state->header_len = snprintf(con_state->headers, sizeof(con_state->headers), HTTP_RESPONSE_REDIRECT_HEADER,
                ipaddr_ntoa(con_state->gw), default_url);
            if(debug_print) { DEBUG_printf("Sending redirect %s", con_state->headers); }
        }
        else
        {
            int target_index = find_first_index(request, ' ', MAX_HEADER_SIZE);
            char small_request[MAX_HEADER_SIZE] = {0};
            if(target_index != -1) memcpy(small_request, request, target_index);
            // Send not found
            con_state->result_len = snprintf(con_state->result, sizeof(NOT_FOUND_BODY), NOT_FOUND_BODY, small_request);
            con_state->header_len = snprintf(con_state->headers, sizeof(con_state->headers), HTTP_RESPONSE_NOT_FOUND_HEADER, con_state->result_len);
            if(debug_print) { DEBUG_printf("Sending not found %s", con_state->headers); }
        }
    }

    // Send the headers to the client
    con_state->sent_len = 0;
    err_t err = tcp_write(pcb, con_state->headers, con_state->header_len, 0);
    if (err != ERR_OK) {
        DEBUG_printf("failed to write header data %d\n", err);
        return tcp_close_client_connection(con_state, pcb, err);
    }

    // Send the body to the client
    if (con_state->result_len) {
        err = tcp_write(pcb, con_state->result, con_state->result_len, 0);
        if (err != ERR_OK) {
            DEBUG_printf("failed to write result data %d\n", err);
            return tcp_close_client_connection(con_state, pcb, err);
        }
    }
    return ERR_OK;
}

err_t ap_tcp_server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg;
    if (!p) {
        if(debug_print) { DEBUG_printf("connection closed\n"); }
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    }
    assert(con_state && con_state->pcb == pcb);
    if (p->tot_len > 0) {
        if(debug_print) { DEBUG_printf("tcp_server_recv %d err %d\n", p->tot_len, err); }
#if 0
        for (struct pbuf *q = p; q != NULL; q = q->next) {
            DEBUG_printf("in: %.*s\n", q->len, q->payload);
        }
#endif
        // Copy the request into the buffer
        pbuf_copy_partial(p, con_state->headers, p->tot_len > sizeof(con_state->headers) - 1 ? sizeof(con_state->headers) - 1 : p->tot_len, 0);

        // Handle GET request
        if (strncmp(HTTP_GET, con_state->headers, sizeof(HTTP_GET) - 1) == 0) {
            char *request = con_state->headers + sizeof(HTTP_GET); // + space
            char *params = strchr(request, '?');
            if (params) {
                if (*params) {
                    char *space = strchr(request, ' ');
                    *params++ = 0;
                    if (space) {
                        *space = 0;
                    }
                } else {
                    params = NULL;
                }
            }

            err_t generate_error = http_get_generate(request, params, pcb, con_state);
            if(generate_error != ERR_OK) return generate_error;
        }
        tcp_recved(pcb, p->tot_len);
    }
    pbuf_free(p);
    return ERR_OK;
}

static err_t ap_tcp_server_poll(void *arg, struct tcp_pcb *pcb) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg;
    DEBUG_printf("tcp_server_poll_fn\n");
    return tcp_close_client_connection(con_state, pcb, ERR_OK); // Just disconnect clent?
}

static void ap_tcp_server_err(void *arg, err_t err) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg;
    if (err != ERR_ABRT) {
        DEBUG_printf("tcp_client_err_fn %d\n", err);
        tcp_close_client_connection(con_state, con_state->pcb, err);
    }
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (err != ERR_OK || client_pcb == NULL) {
        DEBUG_printf("failure in accept\n");
        return ERR_VAL;
    }
    if(debug_print) { DEBUG_printf("client connected\n"); }

    // Create the state for the connection
    TCP_CONNECT_STATE_T *con_state = calloc(1, sizeof(TCP_CONNECT_STATE_T));
    if (!con_state) {
        DEBUG_printf("failed to allocate connect state\n");
        return ERR_MEM;
    }
    con_state->pcb = client_pcb; // for checking
    con_state->gw = &state->gw;

    // setup connection to client
    tcp_arg(client_pcb, con_state);
    tcp_sent(client_pcb, ap_tcp_server_sent);
    tcp_recv(client_pcb, ap_tcp_server_recv);
    tcp_poll(client_pcb, ap_tcp_server_poll, POLL_TIME_S * 2);
    tcp_err(client_pcb, ap_tcp_server_err);

    return ERR_OK;
}

static bool tcp_server_open(void *arg, const char *ap_name) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    DEBUG_printf("starting server on port %d\n", TCP_PORT);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        DEBUG_printf("failed to create pcb\n");
        return false;
    }

    err_t err = tcp_bind(pcb, IP_ANY_TYPE, TCP_PORT);
    if (err) {
        DEBUG_printf("failed to bind to port %d\n",TCP_PORT);
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

    DEBUG_printf("AP website now running, connect to '%s' to access\n", ap_name);
    return true;
}

int access_point_init(const char* access_point_name, const char* access_point_password) {
    state = calloc(1, sizeof(TCP_SERVER_T));
    if (!state) {
        DEBUG_printf("failed to allocate state\n");
        return 1;
    }

    if (cyw43_arch_init()) {
        DEBUG_printf("failed to initialise\n");
        return 1;
    }

    cyw43_arch_enable_ap_mode(access_point_name, access_point_password, CYW43_AUTH_WPA2_AES_PSK);

    ip4_addr_t mask;
    IP4_ADDR(ip_2_ip4(&state->gw), 192, 168, 4, 1);
    IP4_ADDR(ip_2_ip4(&mask), 255, 255, 255, 0);

    // Start the dhcp server    
    dhcp_server_init(&dhcp_server, &state->gw, &mask);

    // Start the dns server    
    dns_server_init(&dns_server, &state->gw);

    if (!tcp_server_open(state, access_point_name)) {
        DEBUG_printf("failed to open server\n");
        return 1;
    }
    return 0;
}

void poll()
{
    // the following #ifdef is only here so this same example can be used in multiple modes;
    // you do not need it in your code
#if PICO_CYW43_ARCH_POLL
    // if you are using pico_cyw43_arch_poll, then you must poll periodically from your
    // main loop (not from a timer interrupt) to check for Wi-Fi driver or lwIP work that needs to be done.
    cyw43_arch_poll();
    // you can poll as often as you like, however if you have nothing else to do you can
    // choose to sleep until either a specified time, or cyw43_arch_poll() has work to do:
    cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
#else
    // if you are not using pico_cyw43_arch_poll, then Wi-FI driver and lwIP work
    // is done via interrupt in the background. This sleep is just an example of some (blocking)
    // work you might be doing.

#endif
}

int access_point_deinit() {
    tcp_server_close(state);
    dns_server_deinit(&dns_server);
    dhcp_server_deinit(&dhcp_server);    
    cyw43_arch_disable_ap_mode();
    // cyw43_arch_deinit(); // this might be needed to completly turn the wifi chip off.
    return 0;
}

int ap_tcp_write(AP_TCP_CONNECTION_T* connection, const char* data, const unsigned int data_len)
{
    TCP_CONNECT_STATE_T* state = (TCP_CONNECT_STATE_T*)connection;
    err_t err = tcp_write(state->pcb, data, data_len, 0);
    if (err != ERR_OK) {
        DEBUG_printf("failed to write header data %d\n", err);
        return tcp_close_client_connection(state, state->pcb, err);
    }
    return err;
}

void ap_register_html_generator(const char *request_str, url_generator_func_t html_generator_func)
{
    register_url(request_str, html_generator_func);
}

bool is_default_requests(const char* request, const unsigned max_size)
{
    (void)max_size;
    int str_one_comp = strncmp(request, DEFAULT_REQUEST_ONE, sizeof(DEFAULT_REQUEST_ONE)-1);
    int str_two_comp = strncmp(request, DEFAULT_REQUEST_TWO, sizeof(DEFAULT_REQUEST_TWO) - 1);
    int str_three_comp = strncmp(request, DEFAULT_REQUEST_THREE, sizeof(DEFAULT_REQUEST_THREE) - 1);

    // printf("req(%s)(one): %d\n", request, str_one_comp);
    // printf("req(%s)(two): %d\n", request, str_two_comp);

    if(str_one_comp == 0) return true;
    if(str_two_comp == 0) return true;
    if(str_three_comp == 0) return true;
    return false;
}

// Function to find the first index of a character in a string
static int find_first_index(const char *str, char ch, unsigned int max_length) {
    // Iterate over the string
    for (unsigned int i = 0; (str[i] != '\0') && (i < max_length); i++) {
        // Check if the current character matches the target character
        if (str[i] == ch) {
            return i; // Return the index if a match is found
        }
    }
    return -1; // Return -1 if the character is not found
}
