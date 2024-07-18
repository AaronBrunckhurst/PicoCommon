#ifndef PICOW_ACCESS_POINT_H
#define PICOW_ACCESS_POINT_H

#include "pico/lock_core.h"
#include "pico/multicore.h"

#ifdef __cplusplus
extern "C" {
#endif

// dont write more then this_value-1 in your generator function, if you make this bigger it will change the internal buffer size, which will work up to an extent, then break because of pico memory limitations.
#define MAX_DATA_SIZE 4096

#define MAX_HEADER_SIZE 128

// if you change this, you will change the default page that clients will be shown. MAKE SURE THIS EXISTS
#define DEFAULT_URL "/index.html"

typedef int (*html_page_generator_func_t)(char *html_page_dst, const unsigned int html_page_dst_max_size, const char *params);

// set this in your program to change what page the users are brought to by default
extern const char* default_url;

// register_html_generator:
// This will register a function to generate an html page when a request is made to the server
// request_str - is the part of the url after the slash.
// html_generator_func - the function that will generate the html page
void register_html_generator(const char *request_str, html_page_generator_func_t html_generator_func);

int access_point_init(const char* access_point_name, const char* access_point_password);
int access_point_deinit();

// poll:
// depending on which cyw43 lib is being used, this may need to be called in your main loop frequently.
void poll();

// useful bits
#define HTTP_REFRESH "<meta http-equiv=\"refresh\" content=\"5\">"

#define EXAMPLE_TEST_BODY "<html><body><h1>Hello from Pico W.</h1><p>Led is %s</p><p><a href=\"?led=%d\">Turn led %s</a></body></html>"

#ifdef __cplusplus
};
#endif

#endif // PICOW_ACCESS_POINT_H