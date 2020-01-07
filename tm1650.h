#pragma once

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

/*
 * Interface logic for TM1650 LED & KeyPad
 *
 * Built from bits of:
 *  - https://os.mbed.com/users/wim/code/TM1650//file/4430a1559b4f/TM1650.cpp (MIT - Wim Huiskamp)
 *  - https://os.mbed.com/users/wim/code/TM1650//file/4430a1559b4f/TM1650.h (MIT - Wim Huiskamp)
 */


#include <stdint.h>
#include <stdbool.h>
#include <mbed.h>

#include "common.h"

#define TM1650_DEF_BRT      3
#define TM1650_COLUMNS      4

class TM1650
{
public:
	TM1650(PinName dio, PinName clk, PinName ain);

	void init(void);

	void setBrightness(int brightness);
	void setDisplay(bool on);

	void clear(void);

	void putc(char c);
	void puts(const char *s);
	void locate(int column);

	int column(void) const { return this->_column; }
	int columns(void) const { return TM1650_COLUMNS; }

	key_event_t getEvent(void);

private:
	void _bufferChar(char c);

	void _pushMem(void);
	void _pushMem(int column);
	void _pushCtrl(void);

	void _busInit(void);
	void _start(void);
	void _stop(void);
	void _write(uint8_t data);

	Timer _keyTimer;
	Timer _interKeyTimer;
	DigitalInOut _dio;
	DigitalOut _clk;
	AnalogIn _ain;

	key_event_t _lastEvent;
	int _column;
	uint8_t _brightness;
	uint8_t _segment;
	uint8_t _display;
	uint8_t _buffer[TM1650_COLUMNS];
};
