#ifndef PICOW_ACCESS_POINT_H
#define PICOW_ACCESS_POINT_H

#include "pico/lock_core.h"
#include "pico/multicore.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AP_TCP_CONNECTION_T_ {
    int dummy_data;
} AP_TCP_CONNECTION_T;

// if you change this, you will change the default page that clients will be shown. MAKE SURE THIS EXISTS
#define DEFAULT_URL "/index.html"

typedef void (*html_page_generator_func_t)(const char *params, AP_TCP_CONNECTION_T* connection, int *write_error_code);

typedef bool (*ap_get_handeler_func_t)(char *request, char *params, AP_TCP_CONNECTION_T* connection, int *write_error_code);

// set this in your program to change what page the users are brought to by default
extern const char* default_url;

// set this if you need to handle get requests in a specific way
// and request endpoint already registered with register_html_generator will not get passed to this function
extern ap_get_handeler_func_t get_handler;

// register_html_generator:
// This will register a function to generate an html page when a request is made to the server
// request_str - is the part of the url after the slash.
// html_generator_func - the function that will generate the html page
void ap_register_html_generator(const char *request_str, html_page_generator_func_t html_generator_func);

int access_point_init(const char* access_point_name, const char* access_point_password);
int access_point_deinit();

// poll:
// depending on which cyw43 lib is being used, this may need to be called in your main loop frequently.
void poll();

int ap_tcp_write(AP_TCP_CONNECTION_T* connection, const char* data, const unsigned int data_len);

// default way to send a get responce
int ap_send_get_responce(AP_TCP_CONNECTION_T* connection, const char* data, const unsigned int data_len);

// useful bits
extern bool debug_print;
#define HTTP_REFRESH "<meta http-equiv=\"refresh\" content=\"5\">"

#define EXAMPLE_TEST_BODY "<html><body><h1>Hello from Pico W.</h1><p>Led is %s</p><p><a href=\"?led=%d\">Turn led %s</a></body></html>"

#ifdef __cplusplus
};
#endif

#endif // PICOW_ACCESS_POINT_H