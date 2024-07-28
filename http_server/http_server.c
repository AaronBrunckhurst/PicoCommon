#include <string.h>

#include "lwip/tcp.h"
#include "tcp_server.h"
#include "http_server.h"
#include "url_collection.h"

#include "lwip/tcp.h"

#define DEBUG_printf printf

#define HTTP_GET "GET"
#define HTTP_GET_CODE 200

#define HTTP_RESPONSE_HEADERS "HTTP/1.1 %d OK\nServer: PICOW (RP2040)\nContent-Length: %d\nContent-Type: text/html; charset=utf-8\nConnection: close\n\n"

#define HTTP_RESPONSE_NOT_FOUND_HEADER "HTTP/1.1 404 Not Found\nServer: PICOW (RP2040)\nContent-Length: %d\nContent-Type: text/html; charset=utf-8\nConnection: close\n\n"
#define NOT_FOUND_BODY "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html>\n<head>\n   <title>404 Not Found</title>\n</head>\n<body>\n   <h1>Not Found</h1>\n   <p>The requested URL \"%s\" was not found on this server.</p>\n</body>\n</html>"

int find_first_index(const char *str, char ch, unsigned int max_length);

bool http_server_debug_prints = false;
const char* default_url = DEFAULT_URL;
ap_get_handeler_func_t get_handler = NULL;

err_t http_send_404(const char *request, TCP_CONNECTION_T* connection)
{
    int target_index = find_first_index(request, ' ', MAX_HEADER_SIZE);
    char small_request_copy[MAX_HEADER_SIZE] = {0};
    if(target_index != -1) memcpy(small_request_copy, request, target_index);
    // Send not found
    char header_404[MAX_HEADER_SIZE] = {0};
    char body_404[MAX_DATA_SIZE] = {0};
    unsigned int body_length = snprintf(body_404, sizeof(body_404), NOT_FOUND_BODY, small_request_copy);
    unsigned int header_length = snprintf(header_404, sizeof(header_404), HTTP_RESPONSE_NOT_FOUND_HEADER, body_length);

    bool header_sucess = tcp_server_send_data(connection, header_404, header_length);
    bool body_sucess = tcp_server_send_data(connection, body_404, body_length);
    if(http_server_debug_prints) { DEBUG_printf("Sending not found %s", header_404); }

    if(header_sucess && body_sucess)
    {
        return ERR_OK;
    }
    else 
    {
        return ERR_ABRT;
    }
}

err_t http_get_generate(const char *request, const char *params, TCP_CONNECTION_T* connection)
{

    if(http_server_debug_prints) {
        DEBUG_printf("Request: %s?%s\n", request, params);
    }

    // Generate content    
    int send_error_code = ERR_OK;
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

    // Send 404 not found
    return http_send_404(request, connection);
}

int handle_tcp_data(const char* data, const unsigned int data_len, TCP_CONNECTION_T* connection)
{
    (void)data_len;
    // Handle GET request
    if (strncmp(HTTP_GET, data, sizeof(HTTP_GET) - 1) == 0) {
        const char *request = data + sizeof(HTTP_GET); // + space
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

        err_t generate_error = http_get_generate(request, params, connection);
        if(generate_error != ERR_OK) return generate_error;
    }
    return ERR_OK;
}

int http_server_start_timeout(const char* wifi_ssid, const char* wifi_password, uint32_t wifi_connect_timeout_ms, const char* hostname, u16_t host_port)
{
    tcp_server_debug_prints = http_server_debug_prints;
    tcp_server_max_read_size = MAX_HEADER_SIZE;
    tcp_server_on_data_recived = handle_tcp_data;
    return tcp_server_start_timeout(wifi_ssid, wifi_password, wifi_connect_timeout_ms, hostname, host_port);
}
int http_server_start(const char* wifi_ssid, const char* wifi_password, const char* hostname, u16_t host_port)
{
    return http_server_start_timeout(wifi_ssid, wifi_password, TCP_SERVER_DEFAULT_WIFI_CONNECT_TIMEOUT_MS, hostname, host_port);
}
int http_server_stop(void)
{ return tcp_server_stop(); }

// Function to find the first index of a character in a string
int find_first_index(const char *str, char ch, unsigned int max_length) {
    // Iterate over the string
    for (unsigned int i = 0; (str[i] != '\0') && (i < max_length); i++) {
        // Check if the current character matches the target character
        if (str[i] == ch) {
            return i; // Return the index if a match is found
        }
    }
    return -1; // Return -1 if the character is not found
}

int html_server_send_get_responce(TCP_CONNECTION_T* connection, const char* data, const unsigned int data_len)
{
    char header_data[MAX_HEADER_SIZE] = {0};
    size_t header_len = snprintf(header_data, sizeof(header_data), HTTP_RESPONSE_HEADERS,
            HTTP_GET_CODE, data_len);
    if (header_len > sizeof(header_data) - 1) {
        DEBUG_printf("Too much header data %d\n", header_len);
        return ERR_CLSD;
    }
    
    // Send the headers to the client
    err_t err_header = tcp_server_send_data(connection, header_data, header_len);

    // Send the body to the client
    err_t err_body = tcp_server_send_data(connection, data, data_len);

    if (err_header != ERR_OK) { return err_header; }
    if (err_body != ERR_OK) { return err_body; }
    return ERR_OK;
}

void html_server_register_generator(const char *request_str, url_generator_func_t html_generator_func)
{
    register_url(request_str, html_generator_func);
}
