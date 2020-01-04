#include <mbed.h>
#include <TinyGPS.h>
#include <millis.h>

#include "common.h"
#include "leds.h"
#include "odom.h"
#include "fs.h"
#include "tm1650.h"
#include "pins.h"

// #define DEBUG_GPS_INPUT

#include "main.h"

const int DISPLAY_MAX_TIME_MS = 100;
const char *ODOM_BIN = "odom.bin";
const double ODOM_MOVING_LOWER_BOUND_MPH = 1.0;
const double ODOM_MOVING_UPPER_BOUND_MPH = 6.0;
const double ODOM_SAVE_DISTANCE_THRESHOLD_M = 50 * METERS_PER_MILE;
const float MIN_TIME_BETWEEN_SAVE_S = 10;
const float MAX_TIME_BETWEEN_SAVE_S = 10 * 60; // 10 minutes

TinyGPS gps;
Serial gps_uart(GPS_TX, GPS_RX); //TODO: PPS? EN?
DigitalOut gps_en(GPS_EN);
Odom odom;
FS fs;
TM1650 tm1650(TM1650_DIO, TM1650_CLK, TM1650_AIN);
Timer display_timer;
Timeout overlay_timer;
Timer save_timer;

volatile bool gps_changed = false;
int display_mode = MODE_SHOW_SPEED;
const mode_func_t mode_func[] = {
    show_speed,
    show_odom,
    show_odom,
    show_odom,
    show_odom,
    show_sats,
    show_dop,
    show_dop,
    show_noop,
};
const char *mode_label[] = {
    "SPD ",
    "LO  ",
    "HI  ",
    "A   ",
    "B   ",
    "SATS",
    "HDOP",
    "PDOP",
    "DBG ",
};
bool have_position = false;
double prev_lat, prev_lon;
double last_save_odom = 0.0;
bool moving = false;
int sats_used, sats_inview;
int pdop, hdop;
bool overlay_visible = false;

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
    tm1650.setDisplay(true);
    display_test();
    tm1650.setBrightness(3);

    display_timer.start();
    save_timer.start();

    if (!fs.init())
        show_error(ERR_DISK);

    load_odom();

    /* TODO:
    * - Hrmm.. min distance threshold currently assumes 10hz, but I think I actually have 1hz? Update code to use time info for reality check.
    * - De-Arduino-ize TinyGPS
    * - Increase baud rate for gps (check datasheet)
    * - Increase fix-rate for gps (check datasheet)
    * - Implement GPS PPS
    * - Allow control of gps from UART0?
    * - Long-press for keys (too easy to reset trip meters)
    */

    set_color(COLOR_OFF);

    while (true) {
        if (save_timer.read() > MAX_TIME_BETWEEN_SAVE_S)
            save_odom();

        if (gps_changed) {
            update_position();
            gps_changed = false;
        }

        if (!overlay_visible && display_timer.read_ms() >= DISPLAY_MAX_TIME_MS) {
            mode_func[display_mode]();
            display_timer.reset();
        }

        input_key_t key = tm1650.getKey();
        if (key != KEY_INVALID)
            handle_key(key);
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
        double speed = gps.d_speed_mph();
        char buf[6];
        if (speed > 999.9)
            speed = 999.9; // Let's... hope not.
        snprintf(buf, sizeof(buf), "%4d", (int)floor(speed));
        tm1650.puts(buf);
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
            if (whole <= 0.0) {
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

    char buf[16];
    if (whole_w > 0 && fract_w > 0)
        snprintf(buf, sizeof(buf), "%*d.%*d", whole_w, (int)(whole) % whole_mod, fract_w, (int)(fract * fract_mod) % fract_mod);
    else if (whole_w == 0)
        snprintf(buf, sizeof(buf), "%*d", fract_w, (int)(fract * fract_mod) % fract_mod);
    else
        snprintf(buf, sizeof(buf), "%*d", whole_w, (int)(whole) % whole_mod);
    tm1650.puts(buf);
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

    char buf[6];
    snprintf(buf, sizeof(buf), "%02d.%02d", sats_used, sats_inview);
    tm1650.puts(buf);
}

void show_dop(void)
{
    int new_hdop, new_pdop;
    new_hdop = gps.hdop();
    new_pdop = gps.pdop();
    if (new_hdop)
        hdop = new_hdop;
    if (new_pdop)
        pdop = new_pdop;

    int dop;
    if (display_mode == MODE_SHOW_HDOP)
        dop = hdop;
    else
        dop = pdop;

    char buf[6];
    snprintf(buf, sizeof(buf), "%04d", dop % 10000);
    tm1650.puts(buf);
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
    char buf[5];
    snprintf(buf, sizeof(buf), "%4d", num);
    tm1650.puts(buf);
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

    if (save_timer.read() < MIN_TIME_BETWEEN_SAVE_S)
        return 1;
    save_timer.reset();

    for (int i = 0; i < ODOM_COUNT; i++)
        o[i] = odom.get_odom((odom_t)i);

    last_save_odom = o[ODOM_ENGINE];

    show_overlay("SAVE", 0.5);

    return fs.write_file(ODOM_BIN, &o, sizeof(o));
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

void handle_key(input_key_t key)
{
    switch (key) {
    case KEY_DOWN:
        if (display_mode < MODE_SHOW_LAST)
            display_mode++;
        show_overlay(mode_label[display_mode], 1.0);
        break;
    case KEY_UP:
        if (display_mode > MODE_SHOW_FIRST)
            display_mode--;
        show_overlay(mode_label[display_mode], 1.0);
        break;
    case KEY_LEFT:
        if (display_mode == MODE_SHOW_TRIP_A)
            odom.reset_odom(ODOM_TRIP_A);
        else if (display_mode == MODE_SHOW_TRIP_B)
            odom.reset_odom(ODOM_TRIP_B);
        save_odom();
        show_overlay("RST ");
        break;
    default:
        break;
    }

    return;
}
