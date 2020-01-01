#include <mbed.h>
#include <TinyGPS.h>
#include <millis.h>

#include "common.h"
#include "leds.h"
#include "odom.h"
#include "fs.h"
#include "tm1650.h"
#include "pins.h"

#define DEBUG_GPS_INPUT

#include "main.h"

const int DISPLAY_MAX_TIME_MS = 100;
const char *ODOM_BIN = "odom.bin";
const double ODOM_MIN_DISTANCE_THRESHOLD_M = 1.0;
const double ODOM_MOVING_DISTANCE_THRESHOLD_M = 2.0;
const double ODOM_SAVE_DISTANCE_THRESHOLD_M = 50 * METERS_PER_MILE;

TinyGPS gps;
Serial gps_uart(GPS_TX, GPS_RX); //TODO: PPS? EN?
DigitalOut gps_en(GPS_EN);
Odom odom;
FS fs;
TM1650 tm1650(TM1650_DIO, TM1650_CLK);
Timer display_timer;

volatile bool gps_changed = false;
int display_mode = MODE_SHOW_SPEED;
mode_func_t mode_func[] = {
    show_speed,
    show_odom,
    show_odom,
    show_odom,
    show_odom,
};
bool have_position = false;
long prev_lat, prev_lon;
double last_save_odom = 0.0;
bool moving = false;

void uart_cb(void)
{
    int c = gps_uart.getc();
#ifdef DEBUG_GPS_INPUT
    fputc(c, stdout);
#endif
    if (gps.encode(c))
        gps_changed = true;
}

int main()
{
    set_color(COLOR_RED);

    startMillis();

    // Let GPS start warming up as soon as possible
    gps_en = 0;
    gps_uart.baud(9600);
    gps_uart.attach(uart_cb);
    gps_en = 1;

    tm1650.init();
    tm1650.setBrightness(3);
    tm1650.setDisplay(true);

    display_test();

    display_timer.start();

    if (!fs.init())
        show_error(ERR_DISK);

    load_odom();

    /* TODO:
    * (Other than testing)
    * - Increase baud rate for gps (check datasheet)
    * - Use "keys" to get to move through display modes
    */

    while (true) {
        if (gps_changed) {
            update_position();

            mode_func[display_mode]();

            gps_changed = false;
            display_timer.reset();
        } else if (display_timer.read_ms() >= DISPLAY_MAX_TIME_MS) {
            mode_func[display_mode]();
            display_timer.reset();
        }
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
    char buf[5];
    snprintf(buf, sizeof(buf), "E %2x", error);
    tm1650.puts(buf);
    set_color(COLOR_RED, 0.25);
    while (1);
}

void show_speed(void)
{
    if (gps.gps_good_data()) {
        float speed = gps.f_speed_mph();
        char buf[6];
        if (speed > 999.9)
            speed = 999.9; // Let's... hope not.
        snprintf(buf, sizeof(buf), "%4d", (int)floor(speed));
        tm1650.puts(buf);
        set_color(COLOR_GREEN, 0.500);
    } else {
        tm1650.puts("----");
        set_color(COLOR_ORANGE, 0.500);
    }
}

void show_odom(void)
{
    odom_t o;
    float dist;
    float fract, whole;
    const char *fmt;

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
    fract = modff(dist, &whole);

    switch (display_mode) {
        case MODE_SHOW_ODOM_HI:
            // Given 123456.78, show 12
            dist = (int)(whole * 0.0001) % 10000;
            fmt = "%4.0f";
            break;
        case MODE_SHOW_ODOM_LO:
            // Given 123456.78, show 3456
            dist = (int)whole % 10000;
            fmt = "%4.0f";
            break;
        case MODE_SHOW_TRIP_A:
            // Given 123456.78, show 456.7
            dist = ((int)whole % 1000) + fract;
            fmt = "%3.1f";
            break;
        case MODE_SHOW_TRIP_B:
            // Given 123456.78, show 456.7
            dist = ((int)whole % 1000) + fract;
            fmt = "%3.1f";
            break;
        default:
            return;
    }

    char buf[16];
    snprintf(buf, sizeof(buf), fmt, dist);
    tm1650.puts(buf);
}

int load_odom(void)
{
    double o[ODOM_COUNT];

    if (!fs.read_file(ODOM_BIN, &o, sizeof(o)))
        return 0;

    last_save_odom = o[ODOM_ENGINE];

    for (int i = 0; i < ODOM_COUNT; i++)
        odom.set_odom((odom_t)i, o[i]);

    return 0;
}

int save_odom(void)
{
    double o[ODOM_COUNT];

    for (int i = 0; i < ODOM_COUNT; i++)
        o[i] = odom.get_odom((odom_t)i);

    last_save_odom = o[ODOM_ENGINE];

    return fs.write_file(ODOM_BIN, &o, sizeof(o));
}

void update_position(void)
{
    long lat, lon;
    unsigned long age;
    double dist_m;

    if (!gps.gps_good_data()) {
        have_position = false;
        return;
    }

    gps.get_position(&lat, &lon, &age);
    if (!have_position) {
        prev_lat = lat;
        prev_lon = lon;
        return;
    }

    if (prev_lat == lat && prev_lon == lon)
        return;

    dist_m = TinyGPS::distance_between(prev_lat, prev_lon, lat, lon);

    prev_lat = lat;
    prev_lon = lon;

    if (moving) {
        if (dist_m < ODOM_MIN_DISTANCE_THRESHOLD_M) {
            save_odom();
            moving = false;
        }
    } else {
        if (dist_m > ODOM_MOVING_DISTANCE_THRESHOLD_M) {
            moving = true;
        }
    }

    if (moving) {
        odom.increment(dist_m);
        if (odom.get_odom(ODOM_ENGINE) - last_save_odom > ODOM_SAVE_DISTANCE_THRESHOLD_M)
            save_odom();
    }
}
