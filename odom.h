#pragma once

#include "common.h"

class Odom
{
public:
    Odom(void);
    
    int load(void);
    void save(void);

    void update_position(long lat, long lon);
    void invalidate_position(void) { this->_have_position = false; };
    
    void reset_odom(odom_t o);
    void set_odom(odom_t o, double dist);
    double get_odom(odom_t o);

private:
    bool _loaded;
    bool _have_position;
    bool _moving;
    long _prev_lat;
    long _prev_lon;
    double _odom[ODOM_COUNT];
    double _last_save_odom;
    dist_unit_t _dist_unit;
};