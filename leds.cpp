#include <mbed.h>

#include "leds.h"

Ticker led_ticker;
DigitalOut led_red(LED_RED);
DigitalOut led_green(LED_GREEN);
DigitalOut led_blue(LED_BLUE);
unsigned int _led_color = ~COLOR_OFF;
bool _blink = false;
float _rate = 0.0;

void _update_leds(void)
{
    _blink = !_blink;
    if (_blink || _rate == 0.0) {
        led_red = _led_color & COLOR_RED;
        led_green = _led_color & COLOR_GREEN;
        led_blue = _led_color & COLOR_BLUE;
    } else {
        led_red = 1;
        led_green = 1;
        led_blue = 1;
    }
}

void set_color(unsigned int c, float rate) {
    if (rate != _rate) {
        if (rate == 0.0)
            led_ticker.detach();
        else
            led_ticker.attach(_update_leds, rate);
    }
    _led_color = ~c;
    _rate = rate;
    if (_rate == 0.0)
       _update_leds();
}

void set_display_mode_led(display_mode_t m)
{
    
}