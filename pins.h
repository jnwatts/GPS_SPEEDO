#pragma once

#include <mbed.h>

#if defined(TARGET_KL05Z)

#define GPS_TX  PTB2  // D0
#define GPS_RX  PTB1  // D1
#define GPS_PPS PTA11 // D2
#define GPS_EN  PTB5  // D3

#define TM1650_DIO PTB4 // D14 (I2C_SDA)
#define TM1650_CLK PTB3 // D15 (I2C_SCL)
#define TM1650_AIN PTB8 // A0 (LED1/LED_RED!!)

// LED_RED = LED1 = A0 = PTB8
// LED_GREEN = LED2 = A1 = PTB9
// LED_BLUE = LED3 = D8 = PTB10

//TODO: Need to see if I can use TM1650 w/ real I2C calls... or else can I multi-use I2C & DigitalIO pins?
#define MMA8451_SDA  PTB4  // D14 (I2C_SDA)
#define MMA8451_SCL  PTB3  // D15 (I2C_SCL)
#define MMA8451_INT1 NC    // --
#define MMA8451_INT2 PTA10 // D4

#define SDIO_CS   PTB11 // D9  (CD)
#define SDIO_MOSI PTA7  // D11 (DI)
#define SDIO_MISO PTA6  // D12 (DO)
#define SDIO_SCLK PTB0  // D13 (CLK)

#elif defined(TARGET_KL25Z)

#define GPS_TX  PTA1  // D0
#define GPS_RX  PTA2  // D1
#define GPS_PPS PTD4  // D2
#define GPS_EN  PTA12 // D3

#define TM1650_DIO PTE0 // D14 (I2C_SDA)
#define TM1650_CLK PTE1 // D15 (I2C_SCL)
#define TM1650_AIN PTB0 // A0 (LED1/LED_RED!!)

// LED_RED   = LED1 = PTB18
// LED_GREEN = LED2 = PTB19
// LED_BLUE  = LED3 = PTD1  = D13

//TODO: Need to see if I can use TM1650 w/ real I2C calls... or else can I multi-use I2C & DigitalIO pins?
#define MMA8451_SDA  PTE25 // --
#define MMA8451_SCL  PTE24 // --
#define MMA8451_INT1 PTA14 // --
#define MMA8451_INT2 PTA15 // --

#define SDIO_CS   PTD5  // D9  (CD)
#define SDIO_MOSI PTD2  // D11 (DI)
#define SDIO_MISO PTD3  // D12 (DO)
#define SDIO_SCLK PTD1  // D13 (CLK)

#else

#error UNDEFINED TARGET

#endif

// 1	CD/DAT3	CS	D9	J7.2
// 2	CMD	DI	D5	J8.6
// 3	GND1	GND	GND	J9.7
// 4	VDD	VDD	3v3	J9.4
// 5	CLK	SCLK	D7	J8.8
// 6	GND2	GND	GND	J9.7
// 7	DAT0	DO	D6	J8.7
// 8	DAT1	N/C
// 9	DAT2	N/C