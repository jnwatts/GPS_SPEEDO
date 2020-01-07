#pragma once

/*
SPDX-License-Identifier: LGPL-2.1-or-later

Copyright (C) 2020 Josh Watts
All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

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
