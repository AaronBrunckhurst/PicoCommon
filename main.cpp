#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "serial_console/serial_console.h"
#include "filesystem/filesystem.h"
#include "ap_website/picow_access_point.h"

#define ACCESS_POINT_NAME "Test Website"
#define ACCESS_POINT_PASSWORD "password"

void prepare_loop(int delay_in_seconds);
void print_status();
void create_status_html_page(const char *params, AP_TCP_CONNECTION_T* connection, int *write_error_code);
void create_index_html_page(const char *params, AP_TCP_CONNECTION_T* connection, int *write_error_code);

int main() {
    //This function must be initialized here or else PicoLogger functions won't work for some reason
    stdio_init_all();

    prepare_loop(7);

    register_default_commands();
    add_command("status", "Prints the status", print_status);
    
    // init filesystem
    init_filesystem();

    // init access point
    if (access_point_init(ACCESS_POINT_NAME, ACCESS_POINT_PASSWORD) != 0) {
        printf("Access point init failed");
        return -1;
    }

    register_html_generator("/status", create_status_html_page);
    register_html_generator("/index.html", create_index_html_page);

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

void print_status() {
    printf("Status: \n");
}

// typedef int (*html_page_generator_func_t)(char *html_page_dst, const unsigned int html_page_dst_max_size, const char *params);

void create_status_html_page(const char *params, AP_TCP_CONNECTION_T* connection, int *write_error_code)
{
    (void)params;

    char html_page[1024] = {0};
    int html_page_used = snprintf(html_page, 1024, "<html><body><h1>Status</h1><p>Some status information</p></body></html>");

    (*write_error_code) = send_get_responce(connection, html_page, html_page_used);
}

void create_index_html_page(const char *params, AP_TCP_CONNECTION_T* connection, int *write_error_code)
{
    (void)params;

    char html_page[1024] = {0};
    int html_page_used = snprintf(html_page, 1024, "<html><body><h1>Status</h1><p>Index: </p></body></html>");

    (*write_error_code) = send_get_responce(connection, html_page, html_page_used);
}
