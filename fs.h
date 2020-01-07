#pragma once

/*
SPDX-License-Identifier: LGPL-2.1-or-later

Copyright (C) 2020 Josh Watts
All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

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