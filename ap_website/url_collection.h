#ifndef URL_COLLECTION_H
#define URL_COLLECTION_H

#include "picow_access_point.h"

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

typedef void (*url_generator_func_t)(const char *params, AP_TCP_CONNECTION_T* connection, int *write_error_code);

void register_url(const char *url, url_generator_func_t html_generator_func);

// create_html_page:
// This function will be called when a request is made to the server
// request - is the part of the url after the slash, you might need that to determine what to do
// params - is the part of the url after the question mark, you might need that to determine what to do
// result - is the buffer where the html page will be stored
// max_result_len - is the maximum length of the result buffer
bool create_html_page(const char *request, const char *params, AP_TCP_CONNECTION_T* connection, int *write_error_code);

#ifdef __cplusplus
};
#endif

#endif // URL_COLLECTION_H