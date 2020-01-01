#pragma once

#include "common.h"

class Odom
{
public:
    Odom(void);

    void increment(double dist);
    void reset_odom(odom_t o);
    void set_odom(odom_t o, double dist);
    double get_odom(odom_t o);

private:
    double _odom[ODOM_COUNT];
    dist_unit_t _dist_unit;
};