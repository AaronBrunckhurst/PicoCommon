#ifndef SERIAL_CONSOLE_H
#define SERIAL_CONSOLE_H

#ifdef __cplusplus
extern "C" {
#endif

#define NEWLINE '\n'
#define COMMAND_DELIMITER_CHAR ' '
#define INPUT_BUFFER_SIZE 1024
#define MAX_COMMAND_NAME_SIZE 64

extern char recive_buffer[INPUT_BUFFER_SIZE];
extern unsigned int recive_buffer_index;

typedef void (*command_function_t)();

typedef struct Command
{
    const char* command_name;
    const char* command_help_str;
    command_function_t func;
} command_t;

//function definitions
// pass NULL to new_command.command_help_str to not print help message.
void register_command(const command_t new_command);
// pass NULL to command_help_str to not print help message.
void add_command(const char* command_name, const char* command_help_str, command_function_t func);
void print_hello_message();
void print_help_message();
void run_command();
void serial_poll();
void register_default_commands();

#ifdef __cplusplus
}
#endif

#endif // SERIAL_CONSOLE_H