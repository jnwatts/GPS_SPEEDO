#pragma once

#include <stdio.h>
#include <stdbool.h>

class UbxParser
{
public:
    UbxParser();

    bool encode(uint8_t c);

    bool got_response(void) {
        bool v = this->_got_response;
        this->_got_response = false;
        return v;
    }

    uint8_t msg_class(void) { return this->_buffer[0]; }
    uint8_t msg_id(void) { return this->_buffer[1]; }
    uint16_t msg_len(void) { return this->_len; }
    const uint8_t *msg_payload(void) { return &this->_buffer[4]; }

private:
    enum ubx_state_t {
        UBX_IDLE,
        UBX_SYNC,
        UBX_PAYLOAD,
    };

    ubx_state_t _state;
    uint16_t _len;
    uint16_t _offset;
    uint8_t _buffer[60];
    int _ck[2];
    volatile bool _got_response;

};
