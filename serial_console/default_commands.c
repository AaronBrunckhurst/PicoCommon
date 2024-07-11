#ifndef SERIAL_CONSOLE_DEFAULT_COMMANDS_H
#define SERIAL_CONSOLE_DEFAULT_COMMANDS_H

#include <stdbool.h>

#include "serial_console.h"

bool keep_captured = false;

/*
printf("? - Print this message.\n");
    printf("echo - print given text.\n");
    printf("status - print information about the current state of this device to console.\n");
    printf("status_malloc - print information about the current state of malloc to console.\n");
    printf("status_sys - print information about the current state of the system to console.\n");
    printf("capture - traps the program flow in a loop (but still alows commands to be run).\n");
    printf("uf2 | bootloader | F - this device will enter bootloader mode.\n");
*/

void run_bootloader_command()
{
    printf("Entering bootloader mode...\n");
    sleep_ms(10);
    reset_usb_boot(0, 0);
}

void run_status_malloc_command()
{    
    print_mallinfo();
}
void run_status_sys_command()
{
    print_current_system_uptime();
    print_flash_usage();
    print_ram_usage();
}

void run_capture_command()
{    
    printf("run_capture_command start.\n");
    recive_buffer_index = 0;
    keep_captured = !keep_captured;
    while(keep_captured)
    {
        watchdog_update();
        serial_poll();
        sleep_ms(100);
    }
    printf("run_capture_command end.\n");
}

// prints the given string to the serial port after the command identifier text.
void run_echo()
{    
    for(unsigned int i = 5; i < recive_buffer_index; i++)
    {
        printf("%c", recive_buffer[i]);
    }
    printf("\n");
}

void register_default_commands()
{
    add_command("?", "Print this message.", print_help_message);
    add_command("help", "Print this message.", print_help_message);
    add_command("echo", "echo - print given text.", run_echo);
    //add_command("status", "status - print information about the current state of this device to console.", run_status_command);
    add_command("status_malloc", "status_malloc - print information about the current state of malloc to console.", run_status_malloc_command);
    add_command("status_sys", "status_sys - print information about the current state of the system to console.", run_status_sys_command);
    add_command("capture", "capture - traps the program flow in a loop (but still alows commands to be run).", run_capture_command);
    add_command("uf2", "uf2 | bootloader | F - this device will enter bootloader mode.", run_bootloader_command);
    add_command("bootloader", "uf2 | bootloader | F - this device will enter bootloader mode.", run_bootloader_command);
    add_command("F", "uf2 | bootloader | F - this device will enter bootloader mode.", run_bootloader_command);
}

#endif // SERIAL_CONSOLE_DEFAULT_COMMANDS_H