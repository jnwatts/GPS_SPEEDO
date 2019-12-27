#pragma once

/* Copyright 2019 Josh Watts
 *
 * SPDX-License-Identifier: MIT
 *
 * Interface logic for TM1650 LED & KeyPad
 *
 * Built from bits of:
 *  - https://os.mbed.com/users/wim/code/TM1650//file/4430a1559b4f/TM1650.cpp (MIT - Wim Huiskamp)
 *  - https://os.mbed.com/users/wim/code/TM1650//file/4430a1559b4f/TM1650.h (MIT - Wim Huiskamp)
 */


#include <stdint.h>
#include <stdbool.h>
#include <mbed.h>

#define TM1650_DEF_BRT      3
#define TM1650_COLUMNS      4

class TM1650
{
public:
	TM1650(PinName dio, PinName clk);

	void init(void);

	void setBrightness(int brightness);
	void setDisplay(bool on);

	void clear(void);

	void putc(char c);
	void puts(const char *s);
	void locate(int column);

	int column(void) const { return this->_column; }
	int columns(void) const { return TM1650_COLUMNS; }

private:
	void _bufferChar(char c);

	void _pushMem(void);
	void _pushMem(int column);
	void _pushCtrl(void);

	void _busInit(void);
	void _start(void);
	void _stop(void);
	void _write(uint8_t data);

	DigitalInOut _dio;
	DigitalOut _clk;

	int _column;
	uint8_t _brightness;
	uint8_t _segment;
	uint8_t _display;
	uint8_t _buffer[TM1650_COLUMNS];
};
