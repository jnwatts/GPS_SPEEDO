/*
SPDX-License-Identifier: BSD-2-Clause

Copyright (c) 2020 Josh Watts. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
