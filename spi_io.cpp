#include <mbed.h>

extern "C" {
#include "spi_io.h"
}
#include "pins.h"

#define USE_SPI

#ifdef USE_SPI

SPI spi(SDIO_MOSI, SDIO_MISO, SDIO_SCLK);
DigitalOut cs(SDIO_CS);
Timer timer;
int timeout_ms;

void SPI_Init (void)
{
    SPI_CS_High();
    spi.format(8, 0);
    spi.frequency(1000000);
}

BYTE SPI_RW (BYTE d)
{
    return spi.write(d);
}

void SPI_Release(void)
{
    WORD idx;
    for (idx=512; idx && (SPI_RW(0xFF)!=0xFF); idx--);
}

void SPI_CS_Low(void)
{
    cs = 0;
}

void SPI_CS_High(void)
{
    cs = 1;
}

void SPI_Freq_High(void)
{
    // 1MHz
    spi.frequency(1000000);
}

void SPI_Freq_Low(void)
{
    // 300kHz
    spi.frequency(300000);
}

void SPI_Timer_On(WORD ms)
{
    timeout_ms = ms;
    timer.reset();
    timer.start();
}

BOOL SPI_Timer_Status(void)
{
    return (timer.read_ms() < timeout_ms);
}

void SPI_Timer_Off(void)
{
    timer.stop();
}

#else

void SPI_Init (void) { }

BYTE SPI_RW (BYTE d) { return 0xFF; }

void SPI_Release(void) { }

void SPI_CS_Low(void) { }

void SPI_CS_High(void) { }

void SPI_Freq_High(void) { }

void SPI_Freq_Low(void) { }

void SPI_Timer_On(WORD ms) { }

BOOL SPI_Timer_Status(void) { return false; }

void SPI_Timer_Off(void) { }

#endif