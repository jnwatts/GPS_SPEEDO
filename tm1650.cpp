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

#include "tm1650.h"

#warning SELECT key is broken until R1 is replaced with 4kOhm
const int THRESHOLD_SELECT = -1;
const int THRESHOLD_RIGHT = 0xFFFF;
const int THRESHOLD_UP = 0xB332;
const int THRESHOLD_LEFT = 0x6666;
const int THRESHOLD_DOWN = 0x2666;
const int KEY_SHORT_PRESS_TIME_MS = 10; // 10ms
const int KEY_LONG_PRESS_TIME_MS = 1000; // 1s
const int INTER_KEY_TIME_MS = 25; // 25ms

#include "ascii_7seg.h"
#define CHARACTERS SevenSegmentASCII
static const int NUM_CHARACTERS = sizeof(CHARACTERS) / sizeof(*CHARACTERS);

static const uint8_t DOT = 0x80;

#define TM1650_SETUP_HOLD_US 1

//Data write command
//Combine with address
//Followed by memory value
#define TM1650_DATA_WR_CMD  0x68

#define TM1650_ADDR_MSK     0x03 //0..3
#define TM1650_ADDR_OFFSET  1

//Display control command
//Followed by brightness, display on/off and segment 7/8
#define TM1650_DSP_CTRL_CMD 0x48

#define TM1650_BRT_MSK      7
#define TM1650_BRT_OFFSET   4

#define TM1650_7_SEG        (1u<<3)
#define TM1650_8_SEG        (0u<<3)

#define TM1650_DSP_ON       (1u<<0)
#define TM1650_DSP_OFF      (0u<<0)

#define TM1650_DEF_BRT      3
#define TM1650_COLUMNS      4

TM1650::TM1650(PinName dio, PinName clk, PinName ain) :
	_dio(dio),
	_clk(clk),
	_ain(ain),
	_column(0)
{

}

void TM1650::init(void)
{
	this->_busInit();

	this->_brightness = TM1650_DEF_BRT;
	this->_segment = TM1650_8_SEG;
	this->_display = TM1650_DSP_OFF;

	this->_pushCtrl();

	this->clear(); // calls _pushMem()
}

void TM1650::setBrightness(int brightness)
{
	if (brightness < 1)
		brightness = 1;

	if (brightness > 8)
		brightness = 8;

	this->_brightness = brightness;

	this->_pushCtrl();
}

void TM1650::setDisplay(bool on)
{
	this->_display = on ? TM1650_DSP_ON : TM1650_DSP_OFF;

	this->_pushCtrl();
}

void TM1650::clear(void)
{
	for (int i = 0; i < TM1650_COLUMNS; i++)
		this->_buffer[i] = 0;
	this->_column = 0;

	this->_pushMem();
}

void TM1650::putc(char c)
{
	this->_bufferChar(c);
	this->_pushMem();
}

void TM1650::puts(const char *s)
{
	this->_column = 0;
	while (*s != '\0' && this->_column < TM1650_COLUMNS)
		this->_bufferChar(*s++);
	this->_pushMem();
}

void TM1650::locate(int column)
{
	if (column < 0)
		column = 0;

	if (column >= TM1650_COLUMNS)
		column = TM1650_COLUMNS - 1;

	this->_column = column;
}

key_event_t TM1650::getEvent(void)
{
	key_event_t event = {KEY_INVALID, ACTION_NONE};
	int time = this->_keyTimer.read_ms();
	uint16_t value = this->_ain.read_u16();

	if (value < THRESHOLD_DOWN)
		event.key = KEY_DOWN;
	else if (value < THRESHOLD_LEFT)
		event.key = KEY_LEFT;
	else if (value < THRESHOLD_UP)
		event.key = KEY_UP;
	else if (value < THRESHOLD_RIGHT)
		event.key = KEY_RIGHT;
	else if (value < THRESHOLD_SELECT)
		event.key = KEY_SELECT;

	if (this->_lastEvent.action == ACTION_RELEASE) {
		time = this->_interKeyTimer.read_ms();
		if (time < INTER_KEY_TIME_MS) {
			goto no_event;
		} else {
			this->_lastEvent = NO_EVENT;
			this->_interKeyTimer.stop();
			this->_interKeyTimer.reset();
		}
	}

	if (event.key != this->_lastEvent.key || event.key == KEY_INVALID) {
		this->_keyTimer.stop();
		this->_keyTimer.reset();

		if (this->_lastEvent.action != ACTION_NONE) {
			this->_lastEvent.key;
			this->_lastEvent.action = ACTION_RELEASE;
			this->_interKeyTimer.start();
			return this->_lastEvent;
		}

		if (event.key != KEY_INVALID)
			this->_keyTimer.start();

		this->_lastEvent = event;
		goto no_event;
	}

	if (this->_lastEvent.action == ACTION_NONE && time >= KEY_SHORT_PRESS_TIME_MS) {
		this->_lastEvent.action = ACTION_PRESS;
		return this->_lastEvent;
	}

	if (this->_lastEvent.action == ACTION_PRESS && time >= KEY_LONG_PRESS_TIME_MS) {
		this->_lastEvent.action = ACTION_LONG_PRESS;
		this->_keyTimer.stop();
		return this->_lastEvent;
	}

no_event:
	return NO_EVENT;
}

void TM1650::_bufferChar(char c)
{
	unsigned int i;

	if (this->_column >= TM1650_COLUMNS)
		return;

	if (c == '.') {
		if (this->_column <= 0)
			return;

		this->_buffer[this->_column - 1] |= DOT;
		return;
	}

	if (c >= 0 && c <= 0xF) {
		i = '0' - ' ' + c;
	} else if (c <= 0x7F) {
		i = c - ' ';
	} else { // '-' and all others
		i = '-' - ' ';
	}

	if (i >= NUM_CHARACTERS)
		i = '-' - ' ';

	this->_buffer[this->_column++] = CHARACTERS[i];
}

void TM1650::_pushMem(void)
{
	for (int i = 0; i < TM1650_COLUMNS; i++)
		this->_pushMem(i);
}

void TM1650::_pushMem(int column)
{
	if (column >= TM1650_COLUMNS)
		return;

	this->_start();
	this->_write(TM1650_DATA_WR_CMD | ((TM1650_ADDR_MSK & column) << TM1650_ADDR_OFFSET));
	this->_write(this->_buffer[column]);
	this->_stop();
}

void TM1650::_pushCtrl(void)
{
	this->_start();
	this->_write(TM1650_DSP_CTRL_CMD);
	this->_write(((this->_brightness & TM1650_BRT_MSK) << TM1650_BRT_OFFSET) | this->_segment | this->_display);
	this->_stop();
}

void TM1650::_busInit(void)
{
	this->_dio.output();
	wait_us(TM1650_SETUP_HOLD_US);

	this->_dio = 1;
	this->_clk = 1;
}

void TM1650::_start(void)
{
	this->_dio = 0;
	wait_us(TM1650_SETUP_HOLD_US);

	this->_clk = 0;
	wait_us(TM1650_SETUP_HOLD_US);
}

void TM1650::_stop(void)
{
	this->_dio=0;
	wait_us(TM1650_SETUP_HOLD_US);

	this->_clk=1;
	wait_us(TM1650_SETUP_HOLD_US);

	this->_dio=1;
	wait_us(TM1650_SETUP_HOLD_US);
}

void TM1650::_write(uint8_t data)
{
	// Write data out MSB first
	for (uint8_t bit = 0x80; bit; bit >>= 1) {
		this->_dio = (data & bit) ? 1 : 0;
		wait_us(TM1650_SETUP_HOLD_US);

		this->_clk = 1;
		wait_us(TM1650_SETUP_HOLD_US);

		this->_clk = 0;
		wait_us(TM1650_SETUP_HOLD_US);
	}

	// Idle and prepare DIO to read data
	this->_dio = 1;
	this->_dio.input();
	wait_us(TM1650_SETUP_HOLD_US);

	// dummy Ack
	this->_clk = 1;
	wait_us(TM1650_SETUP_HOLD_US);
	// ack = this->_dio;

	this->_clk = 0;
	wait_us(TM1650_SETUP_HOLD_US);

	// Return DIO to output mode
	this->_dio.output();
	wait_us(TM1650_SETUP_HOLD_US);

	this->_dio = 1; //idle
}
