#include <mbed.h>
#include <TinyGPS.h>
#include <millis.h>

#include "common.h"
#include "leds.h"
#include "odom.h"
#include "tm1650.h"
#include "pins.h"

void display_test(void);
void show_error(int err);

int display_mode = MODE_SHOW_SPEED;
typedef void (*mode_func_t)(void);
void show_speed(void);
void show_odom(void);
void set_odom(void);
mode_func_t mode_func[] = {
    show_speed,
    show_odom,
    show_odom,
    set_odom,
    set_odom,
    show_odom,
    show_odom,
};

TinyGPS gps;
Serial gps_uart(PTB2, PTB1);
Odom odom;
TM1650 tm1650(TM1650_DIO, TM1650_CLK);
volatile bool gps_changed = false;
Timer display_timer;
const int DISPLAY_MAX_TIME_MS = 100;

void uart_cb(void)
{
    if (gps.encode(gps_uart.getc()))
        gps_changed = true;
}

int main()
{
    set_color(COLOR_RED);

    startMillis();

    tm1650.init();
    tm1650.setBrightness(8);
    tm1650.setDisplay(true);

    display_test();
    
    display_timer.start();

   // gps_uart.baud(9600);
   // gps_uart.attach(uart_cb);

//    /* TODO:
//    * (Other than testing)
//    * - Increase baud rate for gps (check datasheet)
//    * - Use "keys" to get to move through display modes
//    * - Odometer: Test logic & integrate https://os.mbed.com/handbook/SDFileSystem for accessing SDCard.
//    * - Oh... poop... sdfilesystem requires mbed-rtos/os? :-/
//    */
    if (odom.load() < 0) {
        show_error(ERR_DISK);
    }
//
//    while (true) {
//        if (gps_changed) {
//            long lat, lon;
//            unsigned long age;
//
//            gps.get_position(&lat, &lon, &age);
//            if (gps.gps_good_data())
//                odom.update_position(lat, lon);
//            else
//                odom.invalidate_position();
//
//            mode_func[display_mode]();
//            
//            gps_changed = false;
//            display_timer.start();
//        } else if (display_timer.read_ms() >= DISPLAY_MAX_TIME_MS) {
//            mode_func[display_mode]();
//        }
//    }
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
        snprintf(buf, sizeof(buf), "%3.1f", speed);
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
    
void set_odom(void)
{
}

void debug_int(int num, float delay)
{
    char buf[5];
    snprintf(buf, sizeof(buf), "D %2d", num);
    tm1650.puts(buf);
    wait(delay);
}