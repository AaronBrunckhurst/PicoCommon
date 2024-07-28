#include <map>

#include "url_collection.h"
#include "http_server.h"
#include "tcp_server.h"

typedef struct url_item {
    const char *url;
    url_generator_func_t html_generator_func;
} url_item_t;

// <string request_name, url_item_t url_generator>
std::map<const char*, url_item_t> url_map;

extern "C" void register_url(const char *url, url_generator_func_t html_generator_func)
{
    url_item_t item = {url, html_generator_func};
    url_map[url] = item;
}

url_item_t find_handeler(const char *key_or_more)
{
    std::string key_string = std::string(key_or_more);
    for (const auto& [key, value] : url_map) {
        if (key_string.rfind(key, 0) == 0) { // Check if 'text' starts with 'key'
            return value;
        }
    }

    url_item_t return_value = {};
    return return_value;
}


extern "C" bool create_html_page(const char *request, const char *params, TCP_CONNECTION_T* connection, int* write_error_code)
{
    url_item_t handeler = find_handeler(request);

    if (handeler.url == NULL || handeler.html_generator_func == NULL)
    {
        return false;
    }

    handeler.html_generator_func(params, connection, write_error_code);
    return true;
}
