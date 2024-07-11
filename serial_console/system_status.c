#include "pico/stdlib.h"
#include "pico/time.h"
#include <stdio.h>

#include "pico/bootrom.h"
#include "pico/malloc.h"
#include <malloc.h>
#include <inttypes.h>

#include "system_status.h"

void print_flash_usage() {
    // Get the maximum flash size in bytes
    uint32_t flash_size = PICO_FLASH_SIZE_BYTES;//PICO_FLASH_MEM_SIZE_BYTES;

    extern char __flash_binary_end;

    uintptr_t end = (uintptr_t) &__flash_binary_end;
    uintptr_t program_size = end - XIP_BASE;

    // Calculate the used flash memory in bytes
    double flash_used = (program_size / (double)flash_size) * 100.0f;

    // Print the flash usage information
    printf("Currently used flash memory: %.2f%% bytes\n", flash_used);
    printf("Maximum flash memory: %" PRIu32 " bytes\n", flash_size);
}

uint32_t getTotalHeap(void) {
   extern char __StackLimit, __bss_end__;
   
   return &__StackLimit  - &__bss_end__;
}

uint32_t getFreeHeap(void) {
   struct mallinfo m = mallinfo();

   return getTotalHeap() - m.uordblks;
}

void print_mallinfo() {
    struct mallinfo m = mallinfo();

    printf("Total non-mmapped bytes (arena): %d\n", m.arena);
    printf("# of free chunks (ordblks): %d\n", m.ordblks);
    printf("# of free fastbin blocks (smblks): %d\n", m.smblks);
    printf("# of mapped regions (hblks): %d\n", m.hblks);
    printf("Bytes in mapped regions (hblkhd): %d\n", m.hblkhd);
    printf("Max. total allocated space (usmblks): %d\n", m.usmblks);
    printf("Free bytes held in fastbins (fsmblks): %d\n", m.fsmblks);
    printf("Total allocated space (uordblks): %d\n", m.uordblks);
    printf("Total free space (fordblks): %d\n", m.fordblks);
    printf("Top-most, releasable (via malloc_trim) space (keepcost): %d\n", m.keepcost);
}
// max ram: PICO_MAX_UNPROTECTED_RAM_SIZE
void print_ram_usage() {

    unsigned int avalable_ram = getFreeHeap();
    unsigned int total_ram = getTotalHeap();
    double ram_used = 100.0f - ((avalable_ram / (double)total_ram) * 100.0f);

    printf("avalable_ram: %u bytes.\n", avalable_ram);
    printf("total_ram: %u bytes.\n", total_ram);
    printf("ram_used: %.2f%%\n", ram_used);
}

void print_current_system_uptime()
{
    time_t now;
    time(&now);

    // Calculate individual time components
    int seconds = now % 60;
    int minutes = (now / 60) % 60;
    int hours = (now / 3600) % 24;
    int days = now / 86400;

    // Print the current system time in days, hours, minutes, and seconds
    printf("Uptime: %d days, %02d:%02d:%02d\n", days, hours, minutes, seconds);
}