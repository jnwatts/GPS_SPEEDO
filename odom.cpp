#include <TinyGPS.h>
#include <stdio.h>
#include <string.h>
#include <sd-reader/fat.h>
#include <sd-reader/partition.h>
#include <sd-reader/sd_raw.h>

#include "odom.h"

const char *ODOM_FN = "odom.bin";
const double ODOM_MIN_DISTANCE_THRESHOLD_M = 1.0;
const double ODOM_MOVING_DISTANCE_THRESHOLD_M = 2.0;
const double ODOM_SAVE_DISTANCE_THRESHOLD_M = 50 * METERS_PER_MILE;

Odom::Odom(void)
{
    this->_loaded = false;
    this->_have_position = false;
    this->_moving = false;
    this->_prev_lat = this->_prev_lon = TinyGPS::GPS_INVALID_ANGLE;
    this->_odom[ODOM_ENGINE] = 0.0;
    this->_odom[ODOM_TRIP_A] = 0.0;
    this->_odom[ODOM_TRIP_B] = 0.0;
    this->_last_save_odom = 0;
    this->_dist_unit = DIST_MILES;
}

#warning TODO: Move these inside odom class, or at least in to a temporary struct?
static struct partition_struct *partition;
static struct fat_fs_struct *fs;
static struct fat_dir_struct *dd;

static int fs_open(void)
{
    struct fat_dir_entry_struct dir_ent;

    if (!sd_raw_init())
        goto err;

    partition = partition_open(
        sd_raw_read,
        sd_raw_read_interval,
        sd_raw_write,
        sd_raw_write_interval,
        0
    );
    if (!partition) {
        // Maybe no MBR? Try whole disk...
        partition = partition_open(
            sd_raw_read,
            sd_raw_read_interval,
            sd_raw_write,
            sd_raw_write_interval,
            -1
        );

        if (!partition)
            goto err;
    }

    fs = fat_open(partition);
    if (!fs)
        goto err_partition;

    fat_get_dir_entry_of_path(fs, "/", &dir_ent);
    dd = fat_open_dir(fs, &dir_ent);
    if (!dd)
        goto err_fat;

    return 1;

err_fat:
    fat_close(fs);

err_partition:
    partition_close(partition);

err:
    return 0;
}

static void fs_close(void)
{
    /* close file system */
    fat_close(fs);

    /* close partition */
    partition_close(partition);
}

static uint8_t find_file_in_dir(struct fat_fs_struct* fs, struct fat_dir_struct* dd, const char* name, struct fat_dir_entry_struct* dir_entry)
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

static struct fat_file_struct* open_file_in_dir(struct fat_fs_struct* fs, struct fat_dir_struct* dd, const char* name)
{
    struct fat_dir_entry_struct file_entry;
    if(!find_file_in_dir(fs, dd, name, &file_entry))
        return 0;

    return fat_open_file(fs, &file_entry);
}

int Odom::load(void)
{
    struct fat_file_struct *fd;
    intptr_t count;

    // Always init to 0.0
    for (int i = 0; i < ODOM_COUNT; i++)
        this->_odom[i] = 0.0;

    if (!fs_open())
        goto err;

    fd = open_file_in_dir(fs, dd, ODOM_FN);
    // Missing file is ok
    if (!fd)
        goto success;

    count = fat_read_file(fd, (uint8_t*)this->_odom, sizeof(this->_odom));
    // Incomplete read is bad
    if (count != sizeof(this->_odom))
        goto err_file;

    fat_close_file(fd);

success:
    fs_close();

    return 1;

err_file:
    fat_close_file(fd);

err:
    fs_close();

    // In case we got a partial read, init to 0.0 again
    for (int i = 0; i < ODOM_COUNT; i++)
        this->_odom[i] = 0.0;

    return 0;
}

void Odom::save(void)
{
    struct fat_dir_entry_struct file_entry;
    struct fat_file_struct *fd;
    intptr_t count;
    int create_result;

    if (!fs_open())
        goto err;

    if(find_file_in_dir(fs, dd, ODOM_FN, &file_entry)) {
        if(!fat_delete_file(fs, &file_entry))
            goto err;
    }

    create_result = fat_create_file(dd, ODOM_FN, &file_entry);
    if (create_result == 0)
        goto err;

    fd = open_file_in_dir(fs, dd, ODOM_FN);
    // Missing file is bad
    if (!fd)
        goto err;


    count = fat_write_file(fd, (uint8_t*)this->_odom, sizeof(this->_odom));
    // Failure to write is bad
    if (count < 0)
        goto err_file;
    // Failure to write is bad
    if (count != sizeof(this->_odom))
        goto err_file;

    fat_close_file(fd);

    fs_close();

    return;

err_file:
    fat_close_file(fd);

err:
    fs_close();

    return;
}

void Odom::update_position(long lat, long lon)
{
    if (!this->_have_position) {
        this->_prev_lat = lat;
        this->_prev_lon = lon;
        this->_have_position = true;
        return;
    }

    if (this->_prev_lat == lat && this->_prev_lon == lon)
        return;

    double dist_m = TinyGPS::distance_between(this->_prev_lat, this->_prev_lon, lat, lon);

    this->_prev_lat = lat;
    this->_prev_lon = lon;

    if (this->_moving) {
        if (dist_m < ODOM_MIN_DISTANCE_THRESHOLD_M) {
            this->save();
            this->_moving = false;
        }
    } else {
        if (dist_m > ODOM_MOVING_DISTANCE_THRESHOLD_M) {
            this->_moving = true;
        }
    }

    if (this->_moving) {
        this->_odom[ODOM_ENGINE] += dist_m;
        this->_odom[ODOM_TRIP_A] += dist_m;
        this->_odom[ODOM_TRIP_B] += dist_m;
        if (this->_odom[ODOM_ENGINE] - this->_last_save_odom > ODOM_SAVE_DISTANCE_THRESHOLD_M) {
            this->save();
        }
    }
}

void Odom::reset_odom(odom_t o)
{
    if (o != ODOM_ENGINE) {
        this->_odom[o] = 0;
    }
}

void Odom::set_odom(odom_t o, double dist)
{
    if (this->_dist_unit == DIST_MILES)
        dist *= METERS_PER_MILE;
    this->_odom[o] = dist;
}

double Odom::get_odom(odom_t o)
{
    double dist = this->_odom[o];
    if (this->_dist_unit == DIST_MILES)
        dist *= MILES_PER_METER;
    return dist;
}
