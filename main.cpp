#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "serial_console/serial_console.h"
#include "filesystem/filesystem.h"
#include "http_server/http_server.h"
#include "system/temperture.h"

#define WIFI_SSID "2WIRE484"
#define WIFI_PASSWORD "0328357434"
#define HOST_NAME "sensor"

void prepare_loop(int delay_in_seconds);
void print_status();
void print_temp();
void create_status_html_page(const char *params, TCP_CONNECTION_T* connection, int *write_error_code);
void create_index_html_page(const char *params, TCP_CONNECTION_T* connection, int *write_error_code);
void file_html_page(const char *params, TCP_CONNECTION_T* connection, int *write_error_code);

void temp_c_html_page(const char *params, TCP_CONNECTION_T* connection, int *write_error_code);
void temp_f_html_page(const char *params, TCP_CONNECTION_T* connection, int *write_error_code);
void temp_html_page(const char *params, TCP_CONNECTION_T* connection, int *write_error_code);

int main() {
    //This function must be initialized here or else PicoLogger functions won't work for some reason
    stdio_init_all();

    prepare_loop(7);

    temperture_init();

    http_server_debug_prints = true;

    register_default_commands();
    add_command("status", "Prints the status", print_status);
    add_command("temp", "Prints the system temp", print_temp);
    
    // init filesystem
    init_filesystem();

    // init http server
    int status = http_server_start(WIFI_SSID, WIFI_PASSWORD, HOST_NAME, 80);
    printf("tcp_server start status: %d\n", status);
    // if (access_point_init(ACCESS_POINT_NAME, ACCESS_POINT_PASSWORD) != 0) {
    //     printf("Access point init failed");
    //     return -1;
    // }

    html_server_register_generator("/status", create_status_html_page);
    html_server_register_generator("/index.html", create_index_html_page);
    html_server_register_generator("/file", file_html_page);
    html_server_register_generator("/tempC", temp_c_html_page);
    html_server_register_generator("/tempF", temp_f_html_page);
    html_server_register_generator("/temp", temp_html_page);

    const uint LED_PIN = CYW43_WL_GPIO_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    uint32_t prev_time = to_ms_since_boot(get_absolute_time());
    bool led_state = false;

    while (true) {
        serial_poll();

        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        uint32_t elapsed_time = current_time - prev_time;
        if (elapsed_time >= 1000) {
            cyw43_arch_gpio_put(LED_PIN, led_state);  // Set LED state
            led_state = !led_state;        // Toggle LED state
            prev_time = current_time;           
        }
    }
}

void prepare_loop(int delay_in_seconds)
{
    for(int i = 0; i < delay_in_seconds; i++)
    {
        printf("Prepairing %d", i);
        sleep_ms(250);
        printf(".");
        sleep_ms(250);
        printf(".");
        sleep_ms(250);
        printf(".");
        sleep_ms(250);
        printf("\n");
    }
}

void print_temp()
{
    float system_temp_c = read_onboard_temperature_c();
    float system_temp_f = read_onboard_temperature_f();
    printf("Onboard temperature C = %.02f \n", system_temp_c);
    printf("Onboard temperature F = %.02f \n", system_temp_f);
}

void print_status() {
    printf("Status: \n");
}

// typedef int (*html_page_generator_func_t)(char *html_page_dst, const unsigned int html_page_dst_max_size, const char *params);

void temp_c_html_page(const char *params, TCP_CONNECTION_T* connection, int *write_error_code)
{
    (void)params;

    float system_temp_c = read_onboard_temperature_c();

    char html_page[1024] = {0};
    int html_page_used = snprintf(html_page, 1024, "%.02f", system_temp_c);

    (*write_error_code) = html_server_send_get_responce(connection, html_page, html_page_used);
}
void temp_f_html_page(const char *params, TCP_CONNECTION_T* connection, int *write_error_code)
{
    (void)params;

    float system_temp_f = read_onboard_temperature_f();

    char html_page[1024] = {0};
    int html_page_used = snprintf(html_page, 1024, "%.02f", system_temp_f);

    (*write_error_code) = html_server_send_get_responce(connection, html_page, html_page_used);
}

void temp_html_page(const char *params, TCP_CONNECTION_T* connection, int *write_error_code)
{
    (void)params;

    float system_temp_c = read_onboard_temperature_c();
    float system_temp_f = read_onboard_temperature_f();

    char html_page[1024] = {0};
    int html_page_used = snprintf(html_page, 1024, "<html><body><h1>Temperature</h1><p>Onboard temperature C = %.02f</p><p>Onboard temperature F = %.02f</p></body></html>", system_temp_c, system_temp_f);

    (*write_error_code) = html_server_send_get_responce(connection, html_page, html_page_used);
}

void create_status_html_page(const char *params, TCP_CONNECTION_T* connection, int *write_error_code)
{
    (void)params;

    char html_page[1024] = {0};
    int html_page_used = snprintf(html_page, 1024, "<html><body><h1>Status</h1><p>Some status information</p></body></html>");

    (*write_error_code) = html_server_send_get_responce(connection, html_page, html_page_used);
}

void create_index_html_page(const char *params, TCP_CONNECTION_T* connection, int *write_error_code)
{
    (void)params;

    char html_page[1024] = {0};
    int html_page_used = snprintf(html_page, 1024, "<html><body><h1>Status</h1><p>Index: </p></body></html>");

    (*write_error_code) = html_server_send_get_responce(connection, html_page, html_page_used);
}

// Example of a way of downloading files through HTTP
#define HTTP_RESPONSE_FILE_DOWNLOAD_HEADER "HTTP/1.1 200 OK\nServer: PICOW (RP2040)\nContent-Type: text/plain\nContent-Length: %d\nContent-Disposition: attachment; filename=\"%s\"\nConnection: close\n\n"
void file_html_page(const char *params, TCP_CONNECTION_T* connection, int *write_error_code)
{
    (void)params;

    char file_data[128] = {0};
    memset(file_data, 'H', sizeof(file_data));
    unsigned int file_data_len = sizeof(file_data);

    char header_data[256] = {0};
    int header_data_used = snprintf(header_data, 256, HTTP_RESPONSE_FILE_DOWNLOAD_HEADER, file_data_len, "file.txt");


    // Send the headers to the client
    (*write_error_code) = tcp_server_send_data(connection, header_data, header_data_used);
    if ((*write_error_code) != 0) { return; }

    // Send the body to the client
    (*write_error_code) = tcp_server_send_data(connection, file_data, file_data_len);
    if ((*write_error_code) != 0) { return; }
}
