#pragma once

#include <mbed.h>
#include <TinyGPS.h>
#include "UbxParser.h"

class Ublox : public TinyGPS
{
public:
    enum rate_t {
        RATE_1HZ    =  1000, //ms
        RATE_5Hz    =   200, //ms
        RATE_10Hz   =   100, //ms
        RATE_0_33Hz =  3000, //ms
        RATE_0_2Hz  =  5000, //ms
        RATE_0_1Hz  = 10000, //ms
        RATE_0_05Hz = 20000, //ms
    };

    enum dyn_model_t {
        DYN_PORTABLE    = 0, // Portable
        DYN_STATIONARY  = 2, // Stationary
        DYN_PEDESTRIAN  = 3, // Pedestrian
        DYN_AUTOMOTIVE  = 4, // Automotive
        DYN_SEA         = 5, // Sea
        DYN_AIRBORNE_1G = 6, // Airborne with <1g Acceleration
        DYN_AIRBORNE_2G = 7, // Airborne with <2g Acceleration
        DYN_AIRBORNE_4G = 8, // Airborne with <4g Acceleration
    };

    Ublox(PinName TX, PinName RX, PinName EN = NC, PinName PPS = NC);

    void set_enabled(bool enabled);

    bool changed(void);

    void set_feature_rate(const char *feature, int rate);
    void disable_feature(const char *feature);

    void set_baud(int baud);
    void detect_baud(void);

    bool set_fix_rate(uint16_t rate);
    bool set_dyn_model(dyn_model_t dyn_model);

    void reset(void);
    void cold_start(void);
    void hot_start(void);
    void mfr_reset(void);
    void sleep(void);

    void save(void);

protected:
    bool _write_command(uint8_t msg_class, uint8_t msg_id, const uint8_t *payload, uint16_t msg_len);
    void _uart_rx(void);

    Serial _uart;
    DigitalOut _en;
    InterruptIn _pps;
    volatile bool _changed;
    UbxParser _ubx;
};