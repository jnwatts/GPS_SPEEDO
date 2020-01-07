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

#include "UbxParser.h"

UbxParser::UbxParser() :
    _state(UBX_IDLE),
    _len(0),
    _offset(0),
    _got_response(false)
{

}

bool UbxParser::encode(uint8_t c) {
    switch (this->_state) {
        case UBX_IDLE:
            if (c == 0xB5) {
                this->_state = UBX_SYNC;
                this->_got_response = false;
                this->_len = 0;
                this->_offset = 0;
                this->_ck[0] = this->_ck[1] = 0;
            } else {
                return false;
            }
            break;
        case UBX_SYNC:
            if (c == 0x62) {
                this->_state = UBX_PAYLOAD;
            } else {
                this->_state = UBX_IDLE;
                return false;
            }
            break;
        case UBX_PAYLOAD:
            if (this->_offset > sizeof(this->_buffer)){
                this->_state = UBX_IDLE;
                return false;
            }
            this->_buffer[this->_offset++] = c;
            if (this->_offset == 4) {
                this->_len = this->_buffer[2] | this->_buffer[3] << 8;
                if (this->_len > 100) {
                    this->_state = UBX_IDLE;
                    return false;
                }
            }
            if (this->_offset <= this->_len + 4) {
                this->_ck[0] += c;
                this->_ck[1] += this->_ck[0];
            }
            if (this->_offset == this->_len + 6) {
                if (this->_ck[0] == this->_buffer[this->_len + 4]
                    && this->_ck[1] == this->_buffer[this->_len + 5]) {

                    this->_got_response = true;
                }
                this->_state = UBX_IDLE;
            }
            break;
    }
    return true;
}