#pragma once

enum input_key_t {
    KEY_INVALID = 0,
    KEY_DOWN,
    KEY_LEFT,
    KEY_UP,
    KEY_RIGHT,
    KEY_SELECT,
};

enum error_t {
    ERR_DISK = 0x10,
};

enum color_t {
    COLOR_OFF = 0,
    COLOR_RED = 1u<<0,
    COLOR_GREEN = 1u<<1,
    COLOR_BLUE = 1u<<2,
    COLOR_ORANGE = COLOR_RED|COLOR_GREEN,
    COLOR_WHITE = COLOR_RED|COLOR_GREEN|COLOR_BLUE,
};

enum display_mode_t {
    MODE_SHOW_SPEED,
    MODE_SHOW_ODOM_LO,
    MODE_SHOW_ODOM_HI,
    MODE_SHOW_TRIP_A,
    MODE_SHOW_TRIP_B,
    MODE_SHOW_SATS,
    MODE_SHOW_HDOP,
    MODE_SHOW_PDOP,
    MODE_SHOW_DEBUG,

    MODE_SHOW_FIRST = MODE_SHOW_SPEED,
    MODE_SHOW_LAST = MODE_SHOW_PDOP,
};

enum odom_t {
    ODOM_ENGINE = 0,
    ODOM_TRIP_A,
    ODOM_TRIP_B,
    ODOM_COUNT
};

enum dist_unit_t {
    DIST_MILES,
    DIST_KM,
};

static const float MILES_PER_METER = 0.000621371;
static const float METERS_PER_MILE = 1609.34;

void show_debug(int num, float delay = 0.5);