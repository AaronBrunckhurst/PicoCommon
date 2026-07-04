#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define INVALID_FILE -1

void init_filesystem();
void deinit_filesystem();

int get_int(const char* filename);
void set_int(const char* filename, int value);

// returns the length of the read string. -1 if the file doesn't exist
int get_string(const char* filename, char* dst, unsigned int dst_size);
void set_string(const char* filename, const char* value, unsigned int value_length);

// Directory listing
typedef struct {
    char name[256];
    int is_dir;
    unsigned int size;
} file_info_t;

// Callback invoked for each entry. Return false to stop iteration early.
typedef bool (*list_files_cb_t)(const file_info_t* info, void* user_data);

// Enumerate all entries in path, calling cb for each (skipping . and ..).
// Returns the number of entries iterated, or a negative LittleFS error code.
int list_files(const char* path, list_files_cb_t cb, void* user_data);

const char* lfs_error_to_string(const int error);

#ifdef __cplusplus
};
#endif

#endif // FILESYSTEM_H