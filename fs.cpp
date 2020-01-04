#include <string.h>
#include "fs.h"

FS::FS() :
    _partition(nullptr),
    _fs(nullptr),
    _dd(nullptr)
{

}

int FS::init(void)
{
    struct fat_dir_entry_struct dir_ent;

    if (!sd_raw_init())
        goto err;

    this->_partition = partition_open(
        sd_raw_read,
        sd_raw_read_interval,
        sd_raw_write,
        sd_raw_write_interval,
        0
    );
    if (!this->_partition) {
        // Maybe no MBR? Try whole disk...
        this->_partition = partition_open(
            sd_raw_read,
            sd_raw_read_interval,
            sd_raw_write,
            sd_raw_write_interval,
            -1
        );

        if (!this->_partition)
            goto err;
    }

    this->_fs = fat_open(this->_partition);
    if (!this->_fs)
        goto err_partition;

    fat_get_dir_entry_of_path(this->_fs, "/", &dir_ent);
    this->_dd = fat_open_dir(this->_fs, &dir_ent);
    if (!this->_dd)
        goto err_fat;

    return 1;

err_fat:
    fat_close(this->_fs);
    this->_fs = nullptr;

err_partition:
    partition_close(this->_partition);
    this->_partition = nullptr;

err:
    return 0;
}

int FS::write_file(const char *fn, const void *data, size_t size)
{
    struct fat_dir_entry_struct file_entry;

    if (find_file_in_dir(this->_fs, this->_dd, fn, &file_entry))
        if (!fat_delete_file(this->_fs, &file_entry))
            return 0;

    if (!fat_create_file(this->_dd, fn, &file_entry))
        return 0;

    return this->append_file(fn, (uint8_t*)data, size);
}

int FS::append_file(const char *fn, const void *data, size_t size)
{
    struct fat_dir_entry_struct file_entry;
    struct fat_file_struct *fd;
    intptr_t count;
    int32_t offset;

    fd = open_file_in_dir(this->_fs, this->_dd, fn);
    if (!fd) {
        if (!fat_create_file(this->_dd, fn, &file_entry))
            goto err;

        fd = open_file_in_dir(this->_fs, this->_dd, fn);
        if (!fd)
            goto err;
    }

    offset = 0;
    if (!fat_seek_file(fd, &offset, FAT_SEEK_END))
        goto err_file;

    count = fat_write_file(fd, (uint8_t*)data, size);
    // Failure to write is bad
    if (count < 0)
        goto err_file;
    // Failure to write is bad
    if ((size_t)count != size)
        goto err_file;

    fat_close_file(fd);

    return 1;

err_file:
    fat_close_file(fd);

err:
    return 0;
}

int FS::read_file(const char *fn, void *data, size_t size)
{
    struct fat_file_struct *fd;
    intptr_t count;

    fd = open_file_in_dir(this->_fs, this->_dd, fn);
    if (!fd)
        goto err;

    count = fat_read_file(fd, (uint8_t*)data, size);
    if ((size_t)count != size)
        goto err_file;

    fat_close_file(fd);

    return 1;

err_file:
    fat_close_file(fd);

err:
    return 0;
}

int FS::find_file_in_dir(struct fat_fs_struct* fs, struct fat_dir_struct* dd, const char* name, struct fat_dir_entry_struct* dir_entry)
{
    while(fat_read_dir(dd, dir_entry))
    {
        if(strcmp(dir_entry->long_name, name) == 0)
        {
            fat_reset_dir(dd);
            return 1;
        }
    }

    return 0;
}

struct fat_file_struct* FS::open_file_in_dir(struct fat_fs_struct* fs, struct fat_dir_struct* dd, const char* name)
{
    struct fat_dir_entry_struct file_entry;
    if(!find_file_in_dir(fs, dd, name, &file_entry))
        return 0;

    return fat_open_file(fs, &file_entry);
}
