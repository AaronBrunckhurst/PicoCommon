#include "pico_hal.h"
#include "filesystem.h"

int get_int(const char* filename)
{
    int result = 0;
    int file = pico_open(filename, LFS_O_RDONLY);
    if(file < 0)
    {
        printf("Error opening file (%s): %s\n", filename, lfs_error_to_string(filename));
        return INVALID_FILE;
    }
    pico_read(file, &result, sizeof(result));
    pico_close(file);
    return result;
}

void set_int(const char* filename, int value)
{
    int file = pico_open(filename, LFS_O_WRONLY | LFS_O_CREAT);
    if(file < 0)
    {
        printf("Error opening file (%s): %s\n", filename, lfs_error_to_string(filename));
        return;
    }
    pico_write(file, &value, sizeof(value));
    pico_close(file);
}

int get_string(const char* filename, char* dst, unsigned int dst_size)
{
    int file = pico_open(filename, LFS_O_RDONLY);
    if(file < 0)
    {
        printf("Error opening file (%s): %s\n", filename, lfs_error_to_string(filename));
        return INVALID_FILE;
    }
    int result = pico_read(file, dst, dst_size);
    pico_close(file);
    return result;
}
void set_string(const char* filename, const char* value, unsigned int value_length)
{
    int file = pico_open(filename, LFS_O_WRONLY | LFS_O_CREAT);
    if(file < 0)
    {
        printf("Error opening file (%s): %s\n", filename, lfs_error_to_string(filename));
        return;
    }
    pico_write(file, value, value_length);
    pico_close(file);
}

bool format_filesystem()
{
    return false;
}

void init_filesystem()
{
    bool format = format_filesystem();
    int result = pico_mount(format);
    if(result != LFS_ERR_OK)
    {
        printf("Error mounting filesystem: (%d): %s\n", result, lfs_error_to_string(result));
        pico_mount(true); // format the filesystem.
    }

    // Show file system sizes
    struct pico_fsstat_t stat;
    pico_fsstat(&stat);
    printf("FS: blocks %d, block size %d, used %d\n", (int)stat.block_count, (int)stat.block_size,
           (int)stat.blocks_used);
    // increment the boot count with each invocation
    lfs_size_t boot_count;
	// Open (create if doesn't exist) the boot count file
    int file = pico_open("boot_count", LFS_O_RDWR | LFS_O_CREAT);
    boot_count = 0;
	// Read previous boot count. If file was just created read will return 0 bytes
    pico_read(file, &boot_count, sizeof(boot_count));
	// Increment the count
    boot_count += 1;
	// Write it back after seeking to start of file
    pico_rewind(file);
    pico_write(file, &boot_count, sizeof(boot_count));
	// save the file size
	int pos = pico_lseek(file, 0, LFS_SEEK_CUR);
	// Close the file, making sure all buffered data is flushed
    pico_close(file);

    printf("Boot count: %d\n", (int)boot_count);
    printf("File size (should be 4) : %d\n", pos);
}

void deinit_filesystem()
{
    int result = pico_unmount();
    if(result != LFS_ERR_OK)
    {
        printf("Error unmounting filesystem: (%d): %s\n", result, lfs_error_to_string(result));
    }
}

const char* lfs_error_to_string_enum(const enum lfs_error error)
{
    switch(error)
    {
        case LFS_ERR_OK:
            return "No error";
        case LFS_ERR_IO:
            return "Error during device operation";
        case LFS_ERR_CORRUPT:
            return "Corrupted";
        case LFS_ERR_NOENT:
            return "No directory entry";
        case LFS_ERR_EXIST:
            return "Entry already exists";
        case LFS_ERR_NOTDIR:
            return "Entry is not a dir";
        case -6/*LFS_ERR_ISDIR*/:
            return "Entry is a dir";
        case LFS_ERR_NOTEMPTY:
            return "Dir is not empty";
        case LFS_ERR_BADF:
            return "Bad file number";
        case LFS_ERR_FBIG:
            return "File too large";
        case LFS_ERR_INVAL:
            return "Invalid parameter";
        case LFS_ERR_NOSPC:
            return "No space left on device";
        case LFS_ERR_NOMEM:
            return "No more memory available";
        case LFS_ERR_NOATTR:
            return "No data/attr available";
        case LFS_ERR_NAMETOOLONG:
            return "File name too long";
        default:
            return "Unknown error";
    }
}

const char* lfs_error_to_string(const int error)
{
    return lfs_error_to_string_enum((enum lfs_error)error);
}
