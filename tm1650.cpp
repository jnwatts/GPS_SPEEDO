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

#include "tm1650.h"

static const uint8_t CHARACTERS[] = {
	0x3F, ///< 0
	0x06, ///< 1
	0x5B, ///< 2
	0x4F, ///< 3
	0x66, ///< 4
	0x6D, ///< 5
	0x7D, ///< 6
	0x07, ///< 7
	0x7F, ///< 8
	0x6F, ///< 9
	0x77, ///< A
	0x7C, ///< b
	0x39, ///< c
	0x5E, ///< d
	0x79, ///< E
	0x71, ///< F
	0x40, ///< -
	0x00, ///< ' '
};
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

TM1650::TM1650(PinName dio, PinName clk) :
	_dio(dio),
	_clk(clk),
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

void TM1650::_bufferChar(char c)
{
	int i;

	if (this->_column >= TM1650_COLUMNS)
		return;

	if (c == '.') {
		if (this->_column <= 0)
			return;

		this->_buffer[this->_column - 1] |= DOT;
		return;
	}

	if (c >= 0 && c <= 0xF) {
		i = c;
	} else if (c >= '0' && c <= '9') {
		i = (c - '0');
	} else if (c >= 'a' && c <= 'f') {
		i = (c - 'a' + 0xA);
	} else if (c >= 'A' && c <= 'F') {
		i = (c - 'A' + 0xA);
	} else if (c == ' ') {
		i = 17;
	} else { // '-' and all others
		i = 16;
	}

	if (i >= NUM_CHARACTERS)
		i = 16;

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
