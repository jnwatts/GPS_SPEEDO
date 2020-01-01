#pragma once

#include <sd-reader/partition.h>
#include <sd-reader/fat.h>
#include <sd-reader/sd_raw.h>

class FS
{
public:
	FS(void);

	int init(void);

	int write_file(const char *fn, const void *data, size_t size);
	int append_file(const char *fn, const void *data, size_t size);
	int read_file(const char *fn, void *data, size_t size);

private:
	int find_file_in_dir(struct fat_fs_struct* fs, struct fat_dir_struct* dd, const char* name, struct fat_dir_entry_struct* dir_entry);
	struct fat_file_struct* open_file_in_dir(struct fat_fs_struct* fs, struct fat_dir_struct* dd, const char* name);

	struct partition_struct *_partition;
	struct fat_fs_struct *_fs;
	struct fat_dir_struct *_dd;
};