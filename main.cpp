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

#include <mbed.h>
#include <TinyGPS.h>
#include <millis.h>

#include "common.h"
#include "leds.h"
#include "odom.h"
#include "fs.h"
#include "tm1650.h"
#include "pins.h"
#include "ublox.h"

#define PRETTY_LOG

#include "main.h"

const int DISPLAY_MAX_TIME_MS = 100;
const int IDLE_SLEEP_MAX_TIME_MS = 5 * 60 * 1000;
const int MIN_HDOP_THRESHOLD = 500;
const char *ODOM_BIN = "odom.bin";
const char *ODOM_LOG = "odom.log";
const double ODOM_MOVING_LOWER_BOUND_MPH = 1.0;
const double ODOM_MOVING_UPPER_BOUND_MPH = 6.0;
const double ODOM_SAVE_DISTANCE_THRESHOLD_M = 50 * METERS_PER_MILE;
const float MIN_TIME_BETWEEN_SAVE_S = 10;
const float MAX_TIME_BETWEEN_SAVE_S = 10 * 60; // 10 minutes

Serial pc(USBTX, USBRX);
Ublox gps(GPS_TX, GPS_RX, GPS_EN, NC);
Odom odom;
FS fs;
TM1650 tm1650(TM1650_DIO, TM1650_CLK, TM1650_AIN);
Timer display_timer;
Timeout overlay_timer;
Timer save_timer;
Timer idle_timer;

struct {
    mode_func_t func;
    const char *label;
} modes[] = {
    {show_speed,  "SPD "},
    {show_odom,   "LO  "},
    {show_odom,   "HI  "},
    {show_odom,   "A   "},
    {show_odom,   "B   "},
    {show_sats,   "SATS"},
    {show_dop,    "HDOP"},
    {show_dop,    "PDOP"},
    {show_noop,   "DBG "},
};

int display_mode = MODE_SHOW_SATS;
bool sleeping = false;
bool wakeup = false;
bool waiting_for_gps_ready = true;
bool have_position = false;
double prev_lat, prev_lon;
double last_save_odom = 0.0;
bool moving = false;
int sats_used, sats_inview;
int hdop = TinyGPS::GPS_INVALID_HDOP;
int pdop = TinyGPS::GPS_INVALID_PDOP;
bool overlay_visible = false;

char main_buf[72];

int main()
{
    pc.baud(115200);

    set_color(COLOR_RED);

    startMillis();

    tm1650.init();
    tm1650.setDisplay(true);
    display_test();
    tm1650.setBrightness(1);

    display_timer.start();
    save_timer.start();
    idle_timer.start();

    if (!fs.init())
        show_error(ERR_DISK);

    load_odom();

    /* TODO:
    * - Implement GPS PPS (Shoot.. this needs to be moved to PORT A.. (or D? B?))
    * - Detect GPS ready rather than waiting
    */

init_gps:
    // Let GPS start warming up as soon as possible
    gps.set_enabled(true);
    waiting_for_gps_ready = true;
    tm1650.puts("INIT");
    wait(1.0);
    gps.set_baud(115200);
    gps.disable_feature("GLL"); // Not used by TinyGPS
    gps.disable_feature("ZDA"); // Not used by TinyGPS
    gps.disable_feature("VTG"); // Not used by TinyGPS (speed: This could be higher performance way to get speed?)
    gps.set_feature_rate("RMC", 1); // Speed, time, etc
    gps.set_feature_rate("GGA", 5);
    gps.set_feature_rate("GSA", 10);
    gps.set_feature_rate("GSV", 20);
    gps.set_fix_rate(Ublox::RATE_10Hz);
    gps.set_dyn_model(Ublox::DYN_AUTOMOTIVE);
    tm1650.clear();

    // Clear any key events
    tm1650.getEvent();

    set_color(COLOR_OFF);

    while (true) {
        if (!sleeping) {
            if (!waiting_for_gps_ready) {
                if (save_timer.read() > MAX_TIME_BETWEEN_SAVE_S)
                    save_odom();

                if (gps.changed())
                    update_position();
            }

            update_dop();

            if (display_timer.read_ms() >= DISPLAY_MAX_TIME_MS) {
                if (waiting_for_gps_ready)
                    check_for_gps_ready();

                if (!overlay_visible) {
                    modes[display_mode].func();
                    display_timer.reset();
                }
            }

            if (idle_timer.read_ms() >= IDLE_SLEEP_MAX_TIME_MS)
                enter_sleep();
        }

        key_event_t event = tm1650.getEvent();
        if (event != NO_EVENT)
            handle_key_event(event);

        if (wakeup) {
            wakeup = false;
            goto init_gps;
        }

        if (sleeping)
            deepsleep();
        else
            sleep();
    }
}

void display_test(void)
{
    const float delay = 0.1;
    const float delay_b = 0.05;

    tm1650.clear();

    tm1650.setBrightness(8);

    tm1650.locate(0);
    for (int i = 0; i < tm1650.columns(); i++) {
        tm1650.putc('8');
        tm1650.putc('.');
        wait(delay);
    }

    for (int i = 8; i >= 0; i--) {
        tm1650.setBrightness(i);
        wait(delay_b);
    }
    wait(delay);

    for (int i = 0; i <= 8; i++) {
        tm1650.setBrightness(i);
        wait(delay_b);
    }

    tm1650.locate(0);
    for (int i = 0; i < tm1650.columns(); i++) {
        tm1650.putc(' ');
        wait(delay);
    }

    tm1650.clear();
}

void show_error(int error)
{
    snprintf(main_buf, sizeof(main_buf), "E %2x", error);
    tm1650.puts(main_buf);
    set_color(COLOR_RED, 0.25);
    while (1);
}

void show_speed(void)
{
    if (gps.gps_good_data()) {
        double speed = gps.d_speed_mph();
        if (speed > 999.9)
            speed = 999.9; // Let's... hope not.
        snprintf(main_buf, sizeof(main_buf), "%4d", (int)floor(speed));
        tm1650.puts(main_buf);
    } else {
        tm1650.puts("----");
    }
}

void show_odom(void)
{
    odom_t o;
    double dist;
    double whole, fract;
    int whole_w, fract_w;
    int whole_mod, fract_mod;

    switch (display_mode) {
        case MODE_SHOW_ODOM_HI:
            // Given 123456.78, show 12
            o = ODOM_ENGINE;
            break;
        case MODE_SHOW_ODOM_LO:
            // Given 123456.78, show 3456
            o = ODOM_ENGINE;
            break;
        case MODE_SHOW_TRIP_A:
            // Given 123456.78, show 456.7
            o = ODOM_TRIP_A;
            break;
        case MODE_SHOW_TRIP_B:
            // Given 123456.78, show 456.7
            o = ODOM_TRIP_B;
            break;
        default:
            return;
    }

    dist = odom.get_odom(o);
    fract = modf(dist, &whole);

    switch (display_mode) {
        case MODE_SHOW_ODOM_HI:
            // Given 123456.78, show 12
            whole *= 0.0001;
            if (whole < 1.0) {
                tm1650.clear();
                return;
            }
            whole_w = 4;
            fract_w = 0;
            break;
        case MODE_SHOW_ODOM_LO:
            // Given 123456.78, show 3456
            whole_w = 4;
            fract_w = 0;
            break;
        case MODE_SHOW_TRIP_A:
            // Given 123456.78, show 456.7
            whole_w = 3;
            fract_w = 1;
            break;
        case MODE_SHOW_TRIP_B:
            // Given 123456.78, show 456.7
            whole_w = 3;
            fract_w = 1;
            break;
        default:
            return;
    }

    whole_mod = (int)pow(10, whole_w);
    fract_mod = (int)pow(10, fract_w);

    if (whole_w > 0 && fract_w > 0)
        snprintf(main_buf, sizeof(main_buf), "%*d.%*d", whole_w, (int)(whole) % whole_mod, fract_w, (int)(fract * fract_mod) % fract_mod);
    else if (whole_w == 0)
        snprintf(main_buf, sizeof(main_buf), "%*d", fract_w, (int)(fract * fract_mod) % fract_mod);
    else
        snprintf(main_buf, sizeof(main_buf), "%*d", whole_w, (int)(whole) % whole_mod);
    tm1650.puts(main_buf);
}

void show_sats(void)
{
    int new_sats_used, new_sats_inview;
    new_sats_used = gps.satsused();
    new_sats_inview = gps.satsinview();

    if (new_sats_used)
        sats_used = new_sats_used;
    if (new_sats_inview)
        sats_inview = new_sats_inview;

    snprintf(main_buf, sizeof(main_buf), "%02d.%02d", sats_used, sats_inview);
    tm1650.puts(main_buf);
}

void show_dop(void)
{
    int dop;
    char label;
    if (display_mode == MODE_SHOW_HDOP) {
        label = 'H';
        dop = hdop;
    } else {
        label = 'P';
        dop = pdop;
    }

    snprintf(main_buf, sizeof(main_buf), "%c%03d", label, dop % 10000);
    tm1650.puts(main_buf);
}

void show_noop(void)
{
    return;
}

void show_overlay(const char *msg, float delay)
{
    tm1650.puts(msg);
    overlay_visible = true;
    overlay_timer.attach(hide_overlay, delay);
}

void hide_overlay(void)
{
    overlay_visible = false;
}

void show_debug(int num, float delay)
{
    snprintf(main_buf, sizeof(main_buf), "%4d", num);
    tm1650.puts(main_buf);
    if (delay > 0.0)
        wait(delay);
    display_mode = MODE_SHOW_DEBUG;
}

int load_odom(void)
{
    double o[ODOM_COUNT];

    if (!fs.read_file(ODOM_BIN, &o, sizeof(o))) {
        show_overlay("DISK", 1.0);
        wait(1.0);
        show_overlay("FAIL", 1.0);

        for (int i = 0; i < ODOM_COUNT; i++)
            odom.set_odom((odom_t)i, 0.0);

        return 0;
    }

    last_save_odom = o[ODOM_ENGINE];

    for (int i = 0; i < ODOM_COUNT; i++)
        odom.set_odom((odom_t)i, o[i]);

    return 0;
}

int save_odom(void)
{
    double o[ODOM_COUNT];
    int result;

    if (save_timer.read() < MIN_TIME_BETWEEN_SAVE_S)
        return 1;
    save_timer.reset();

    for (int i = 0; i < ODOM_COUNT; i++)
        o[i] = odom.get_odom((odom_t)i);

    last_save_odom = o[ODOM_ENGINE];

    if (!overlay_visible)
        show_overlay("SAVE", 0.5);

#ifdef PRETTY_LOG
    {
        int buf_len;
        unsigned long age;
        int year;
        uint8_t month, day, hour, minute, second, hundredths;
        gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
        if (year < 2020)
            year += 20; // Roll-over for if this firmware is still in use 20 years from now. *snicker*
        buf_len = snprintf(main_buf, sizeof(main_buf),
            "%04d-%02d-%02d %02d:%02d:%02d.%03d+%03lu, %f, %f, %f\n",
            year, month, day,
            hour, minute, second, hundredths, age,
            o[ODOM_ENGINE],
            o[ODOM_TRIP_A],
            o[ODOM_TRIP_B]
        );
        fs.append_file(ODOM_LOG, main_buf, buf_len);
    }
#endif

    result = fs.write_file(ODOM_BIN, &o, sizeof(o));
    if (!result) {
        show_overlay("DISK", 1.0);
        wait(1.0);
        show_overlay("FAIL", 1.0);
    }
    return result;
}

void update_position(void)
{
    double lat, lon;
    unsigned long age;
    double dist_m;
    double speed_mph;

    if (!gps.gps_good_data()) {
        have_position = false;
        return;
    }

    gps.d_get_position(&lat, &lon, &age);
    if (!have_position) {
        prev_lat = lat;
        prev_lon = lon;
        have_position = true;
        return;
    }

    if (prev_lat == lat && prev_lon == lon)
        return;

    dist_m = TinyGPS::distance_between(prev_lat, prev_lon, lat, lon);
    speed_mph = gps.d_speed_mph();

    prev_lat = lat;
    prev_lon = lon;

    if (moving) {
        idle_timer.reset();
        if (speed_mph < ODOM_MOVING_LOWER_BOUND_MPH) {
            save_odom();
            moving = false;
        }
    } else {
        if (speed_mph > ODOM_MOVING_UPPER_BOUND_MPH) {
            moving = true;
        }
    }

    if (moving) {
        odom.increment(dist_m);
        if (odom.get_odom(ODOM_ENGINE) - last_save_odom > ODOM_SAVE_DISTANCE_THRESHOLD_M)
            save_odom();
    }
}

void update_dop(void)
{
    int new_dop;

    new_dop = gps.hdop();
    if (new_dop)
        hdop = new_dop;

    new_dop = gps.pdop();
    if (new_dop)
        pdop = new_dop;
}

void handle_key_event(key_event_t event)
{
    idle_timer.reset();

    if (sleeping) {
        if (event.key == KEY_RIGHT && event.action == ACTION_PRESS)
            exit_sleep();
        return;
    }

    switch (event.key) {
    case KEY_DOWN:
        if (event.action == ACTION_PRESS) {
            if (display_mode < MODE_SHOW_LAST)
                display_mode++;
            show_overlay(modes[display_mode].label, 1.0);
        }
        break;
    case KEY_UP:
        if (event.action == ACTION_PRESS) {
            if (display_mode > MODE_SHOW_FIRST)
                display_mode--;
            show_overlay(modes[display_mode].label, 1.0);
        }
        break;
    case KEY_LEFT:
        if (event.action == ACTION_LONG_PRESS) {
            if (display_mode == MODE_SHOW_TRIP_A)
                odom.reset_odom(ODOM_TRIP_A);
            else if (display_mode == MODE_SHOW_TRIP_B)
                odom.reset_odom(ODOM_TRIP_B);
            show_overlay("RST ");
            save_odom();
        }
        break;
    case KEY_RIGHT:
        if (event.action == ACTION_LONG_PRESS && !sleeping)
            enter_sleep();
    default:
        break;
    }

    return;
}

void check_for_gps_ready(void)
{
    long lat, lon;
    unsigned long age_ms;

    gps.get_position(&lat, &lon, &age_ms);
    if (age_ms != TinyGPS::GPS_INVALID_AGE && age_ms > 10*1000) {
        show_overlay("EFIX");
        return;
    }

    if (hdop <= MIN_HDOP_THRESHOLD && gps.gps_good_data()) {
        waiting_for_gps_ready = false;
        display_mode = MODE_SHOW_SPEED;
    }
}

void enter_sleep(void)
{
    idle_timer.reset();
    gps.set_enabled(false);
    tm1650.puts("SLP ");
    wait(0.75);
    set_color(COLOR_OFF);
    tm1650.clear();
    sleeping = true;
    wait(1.0);
}

void exit_sleep(void)
{
    wakeup = true;
    sleeping = false;
    idle_timer.reset();
    display_timer.reset();
    save_timer.reset();
}
