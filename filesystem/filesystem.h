#ifndef FILESYSTEM_H
#define FILESYSTEM_H

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

const char* lfs_error_to_string(const int error);

#ifdef __cplusplus
};
#endif

#endif // FILESYSTEM_H