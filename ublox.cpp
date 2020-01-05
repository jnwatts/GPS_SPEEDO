#include <stdio.h>
#include "ublox.h"

// #define GPS_UART_PASSTHROUGH

Ublox::Ublox(PinName TX, PinName RX, PinName EN, PinName PPS) :
    _uart(TX, RX),
    _en(EN),
    _pps(PPS),
    _changed(false)
{
    this->set_enabled(false);
    this->_uart.baud(9600);
    this->_uart.attach(this, &Ublox::_uart_rx);
}

void Ublox::set_enabled(bool enabled)
{
    this->_en = enabled ? 1 : 0;
}

bool Ublox::changed(void)
{
    bool changed = this->_changed;
    this->_changed = false;
    return changed;
}

//TODO: CFG-NAV5 -> Set to automotive? :-D

void Ublox::set_feature_rate(const char *feature, int rate)
{
    uint8_t payload[3];

    static struct feature_t {
        char mnemonic[4];
        uint8_t class_id[2];
    } features[] = {
        {"DTM", {0xF0, 0x0A}}, // Datum Reference
        {"GBS", {0xF0, 0x09}}, // GNSS Satellite Fault Detection
        {"GGA", {0xF0, 0x00}}, // Global positioning system fix data
        {"GLL", {0xF0, 0x01}}, // Latitude and longitude, with time of position fix and status
        {"GLQ", {0xF0, 0x43}}, // Poll a standard message (if the current Talker ID is GL)
        {"GNQ", {0xF0, 0x42}}, // Poll a standard message (if the current Talker ID is GN)
        {"GNS", {0xF0, 0x0D}}, // GNSS fix data
        {"GPQ", {0xF0, 0x40}}, // Poll a standard message (if the current Talker ID is GP)
        {"GRS", {0xF0, 0x06}}, // GNSS Range Residuals
        {"GSA", {0xF0, 0x02}}, // GNSS DOP and Active Satellites
        {"GST", {0xF0, 0x07}}, // GNSS Pseudo Range Error Statistics
        {"GSV", {0xF0, 0x03}}, // GNSS Satellites in View
        {"RMC", {0xF0, 0x04}}, // Recommended Minimum data
        {"TXT", {0xF0, 0x41}}, // Text Transmission
        {"VTG", {0xF0, 0x05}}, // Course over ground and Ground speed
        {"ZDA", {0xF0, 0x08}}, // Time and Date
    };

    for (unsigned int i = 0; i < sizeof(features) / sizeof(*features); i++) {
        feature_t *f = &features[i];
        if (strncasecmp(feature, f->mnemonic, 3) == 0) {
            payload[0] = f->class_id[0];
            payload[1] = f->class_id[1];
            payload[2] = rate;
            this->_write_command(0x06, 0x01, payload, 3);
            return;
        }
    }

    return;
}

void Ublox::disable_feature(const char *feature)
{
    this->set_feature_rate(feature, 0);
}

void Ublox::set_baud(int baud)
{
    uint8_t payload[20];

    uint8_t port = 1;
    uint16_t txReady = 0;
    uint32_t mode = 0x000008D0;
    uint16_t inProtoMask = 0x0007;
    uint16_t outProtoMask = 0x0003;
    uint16_t flags = 0;

    payload[0]  = port;
    payload[1]  = 0; // reserved
    payload[2]  = txReady;
    payload[3]  = txReady >> 8;
    payload[4]  = mode;
    payload[5]  = mode >> 8;
    payload[6]  = mode >> 16;
    payload[7]  = mode >> 24;
    payload[8]  = baud;
    payload[9]  = baud >> 8;
    payload[10] = baud >> 16;
    payload[11] = baud >> 24;
    payload[12] = inProtoMask;
    payload[13] = inProtoMask >> 8;
    payload[14] = outProtoMask;
    payload[15] = outProtoMask >> 8;
    payload[16] = flags;
    payload[17] = flags >> 8;
    payload[18] = 0; // reserved
    payload[19] = 0; // reserved

    if (this->_write_command(0x06, 0x00, payload, 20)) {
        this->_write_command(0x06, 0x00, payload, 0);
    }
    this->_uart.baud(baud);
}

bool Ublox::set_fix_rate(uint16_t rate)
{
    uint8_t payload[6];
    
    uint16_t navRate = 1; // cycle, must be 1
    uint16_t timeRef = 0; // 0 - utc, 1 - GPS

    payload[0] = rate;
    payload[1] = rate >> 8;
    payload[2] = navRate;
    payload[3] = navRate >> 8;
    payload[4] = timeRef;
    payload[5] = timeRef >> 8;

    return this->_write_command(0x06, 0x08, payload, 6);
}

bool Ublox::set_dyn_model(dyn_model_t dyn_model)
{
    uint8_t payload[36];

    memset(payload, 0, 36);

    uint16_t mask = 1u<<0; // apply dyn

    payload[0] = mask;
    payload[1] = mask >> 8;
    payload[2] = dyn_model;

    return this->_write_command(0x06, 0x24, payload, 36);
}

bool Ublox::_write_command(uint8_t msg_class, uint8_t msg_id, const uint8_t *payload, uint16_t msg_len)
{
    uint8_t CK_A, CK_B;
    uint8_t header[4];
    unsigned int i;

    CK_A = CK_B = 0;

    this->_uart.putc(0xB5);
    this->_uart.putc(0x62);

    header[0] = msg_class;
    header[1] = msg_id;
    header[2] = msg_len;
    header[3] = msg_len >> 8;

    for (i = 0; i < sizeof(header); i++) {
        CK_A += header[i];
        CK_B += CK_A;
        this->_uart.putc(header[i]);
    }

    for (i = 0; i < msg_len; i++) {
        CK_A += payload[i];
        CK_B += CK_A;
        this->_uart.putc(payload[i]);
    }

    this->_uart.putc(CK_A);
    this->_uart.putc(CK_B);

    if (msg_class == 0x06) {
        Timer msg_timeout;
        msg_timeout.start();

        while (!this->_ubx.got_response()) {
            if (msg_timeout.read_ms() > 500)
                return false;
        }
    }

    return true;
}

void Ublox::_uart_rx(void)
{
    int c = this->_uart.getc();
#ifdef GPS_UART_PASSTHROUGH
    fputc(c, stdout);
#endif
    if (this->_term_offset == 0 && this->_ubx.encode(c)) {
        return;
    } else {
        if (this->encode(c))
            this->_changed = true;
    }
}