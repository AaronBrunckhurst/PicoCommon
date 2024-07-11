#ifndef SYSTEM_STATUS_H
#define SYSTEM_STATUS_H

#ifdef __cplusplus
extern "C" {
#endif

void print_current_system_uptime();
void print_flash_usage();
void print_ram_usage();
void print_mallinfo();

#ifdef __cplusplus
};
#endif

#endif // SYSTEM_STATUS_H