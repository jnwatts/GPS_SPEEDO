#include <stdio.h>
#include <mbed.h>

#include "spi_io.h"
#include "common.h"
#include "pins.h"

SPI spi(SDIO_MOSI, SDIO_MISO, SDIO_SCLK);
DigitalOut cs(SDIO_CS);
Timer timer;
int timeout_ms;
bool timer_en = false;

void SPI_Init (void)
{
    SPI_CS_High();
    SPI_Freq_Low();
    spi.format(8, 0);
}

unsigned int SPI_RW (unsigned int d)
{
    return spi.write(d);
}

void SPI_Release(void)
{
    int idx;
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

void SPI_Timer_On(int ms)
{
    timeout_ms = ms;
    timer_en = true;
    timer.reset();
    timer.start();
}

bool SPI_Timer_Status(void)
{
    return (timer.read_ms() < timeout_ms) && timer_en;
}

void SPI_Timer_Off(void)
{
    timer_en = false;
    timer.stop();
}
