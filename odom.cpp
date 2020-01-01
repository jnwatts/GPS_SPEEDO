#include <TinyGPS.h>
#include <stdio.h>
#include <string.h>
#include <sd-reader/fat.h>
#include <sd-reader/partition.h>
#include <sd-reader/sd_raw.h>

#include "odom.h"

Odom::Odom(void)
{
    this->_odom[ODOM_ENGINE] = 0.0;
    this->_odom[ODOM_TRIP_A] = 0.0;
    this->_odom[ODOM_TRIP_B] = 0.0;
    this->_dist_unit = DIST_MILES;
}

void Odom::increment(double dist_m)
{
    this->_odom[ODOM_ENGINE] += dist_m;
    this->_odom[ODOM_TRIP_A] += dist_m;
    this->_odom[ODOM_TRIP_B] += dist_m;
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
