#ifndef NETWORKING_PICO_SERIAL_CONSOLE_CPP
#define NETWORKING_PICO_SERIAL_CONSOLE_CPP

#include <stdio.h>
#include <string.h>
#include <ctype.h> // For isprint
#include <map>
#include <string>
#include <iostream>

#include "pico/stdlib.h"
#include "hardware/watchdog.h"
#include "pico/bootrom.h"

#include "system_status.h"
#include "serial_console.h"

extern "C" {
    #include "default_commands.c"
}

#define ERROR_CHAR 255

char recive_buffer[INPUT_BUFFER_SIZE];
unsigned int recive_buffer_index = 0;

std::map<std::string, command_t> commands;

extern "C" void register_command(const command_t command)
{
    commands[command.command_name] = command;
}

extern "C" void add_command(const char* command_name, const char* command_help_str, command_function_t func)
{
    command_t new_command;
    new_command.command_name = command_name;
    new_command.command_help_str = command_help_str;
    new_command.func = func;

    register_command(new_command);
}

extern "C" void print_hello_message()
{
    printf("Pico now accepting commands on this serial port, type \"?\" for a list of commands.\n");
}

extern "C" void print_help_message()
{
    printf("Commands:\n");

    std::map<std::string, command_t>::iterator it = commands.begin();

    // Iterate through the map and print the elements
    while (it != commands.end()) {
        std::cout << it->first << " - " << it->second.command_help_str << std::endl;
        ++it;
    }
}

extern "C" void serial_poll()
{
    // recive serial input
    char read_char = getchar_timeout_us(1);
    while ((read_char != PICO_ERROR_TIMEOUT) && (read_char != ERROR_CHAR))
    {
        //printf("serial_poll() <-\"%c\"|\"%d\" is \"%d\"\n", read_char, read_char, PICO_ERROR_TIMEOUT);
        if (isprint(read_char) || (read_char == NEWLINE))
        {
            if(recive_buffer_index >= INPUT_BUFFER_SIZE)
            {
                recive_buffer_index = 0;
            }

            if(read_char == NEWLINE)
            {
                recive_buffer[recive_buffer_index] = '\0';
                printf("Recived: \"%s\".\n", recive_buffer);
                if(recive_buffer_index != 0) {                    
                    run_command();          
                    recive_buffer_index = 0;          
                }            
            }
            // only record the char if it is a valid char
            else {
                //printf("read_char1: %c\n", read_char);
                recive_buffer[recive_buffer_index] = read_char;            
                ++recive_buffer_index;
                recive_buffer[recive_buffer_index] = '\0';
            }
        }

    read_char = getchar_timeout_us(1);
    }   
}

bool first_index_of(const char* buffer, const unsigned int buffer_size, const char search_char, unsigned int* index)
{
    for(unsigned int i = 0; i < buffer_size; i++)
    {
        if(buffer[i] == '\0') { return false; }

        if(buffer[i] == search_char)
        {
            *index = i;
            return true;
        }
    }
    return false;
}

extern "C" void run_command()
{
    unsigned int first_arg_end = 0;
    char arg_string[MAX_COMMAND_NAME_SIZE];
    memset(arg_string, 0, MAX_COMMAND_NAME_SIZE);

    // Extract the command argument from the recive_buffer string.
    bool arg_found = first_index_of(recive_buffer, INPUT_BUFFER_SIZE, COMMAND_DELIMITER_CHAR, &first_arg_end);

    // If the command argument is not found, then we are experencing an error.
    if(!arg_found)
    {
        printf("Command parse argument error.\n");
        recive_buffer_index = 0;
        return;
    }

    printf("Command: \"%s\".\n", arg_string);

    auto found_command = commands.find(arg_string);

    // Handle if the command is not found.
    if (found_command == commands.end())
    {
        printf("Command \"%s\" not recognized.\n", arg_string);
        recive_buffer_index = 0;
        return;
    }

    found_command->second.func();

    recive_buffer_index = 0;
}

#endif // NETWORKING_PICO_SERIAL_CONSOLE_CPP
