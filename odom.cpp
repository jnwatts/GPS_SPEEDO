#include <TinyGPS.h>

#include "odom.h"
#include "pff.h"

FATFS fs;
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

int Odom::load(void)
{
    UINT br;
    int err;

    if (this->_loaded)
        return 0;

    err = pf_mount(&fs);
    debug_int(err, 1.0);
    if (err != FR_OK)
        return -1;

    // Failure to open is ok: We might not have saved yet
    if (pf_open(ODOM_FN) != FR_OK)
        return 0;

    // Same for reading (probably)
    if (pf_read(this->_odom, sizeof(this->_odom), &br) != FR_OK)
        return 0;

    return 0;
}

void Odom::save(void)
{
    UINT bw;

    if (pf_open(ODOM_FN) != FR_OK)
        return;

    if (pf_write(this->_odom, sizeof(this->_odom), &bw) != FR_OK)
        return;
        
    this->_last_save_odom = this->_odom[ODOM_ENGINE];
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
