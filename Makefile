###############################################################################
# Project settings

PROJECT := GPS_SPEEDO
TARGET_CPU := KL25Z
# TARGET_CPU := KL05Z
MOUNT_POINT := /m/FRDM-KL25Z

# Project settings
###############################################################################
# Objects and Paths

SRC += Arduino.cpp
SRC += leds.cpp
SRC += main.cpp
SRC += odom.cpp
SRC += spi_io.cpp
SRC += TinyGPS.cpp
SRC += tm1650.cpp
SRC += millis/millis.cpp
SRC += pff/diskio.c
SRC += pff/pff.c
SRC += ulibSD/sd_io.c

API_DIR := mbed/common/

SYS_SRC += $(API_DIR)assert.c
SYS_SRC += $(API_DIR)board.c
SYS_SRC += $(API_DIR)BusIn.cpp
SYS_SRC += $(API_DIR)BusInOut.cpp
SYS_SRC += $(API_DIR)BusOut.cpp
SYS_SRC += $(API_DIR)CallChain.cpp
SYS_SRC += $(API_DIR)CAN.cpp
SYS_SRC += $(API_DIR)error.c
SYS_SRC += $(API_DIR)Ethernet.cpp
SYS_SRC += $(API_DIR)FileBase.cpp
SYS_SRC += $(API_DIR)FileLike.cpp
SYS_SRC += $(API_DIR)FilePath.cpp
SYS_SRC += $(API_DIR)FileSystemLike.cpp
SYS_SRC += $(API_DIR)gpio.c
SYS_SRC += $(API_DIR)I2C.cpp
SYS_SRC += $(API_DIR)I2CSlave.cpp
SYS_SRC += $(API_DIR)InterruptIn.cpp
SYS_SRC += $(API_DIR)InterruptManager.cpp
SYS_SRC += $(API_DIR)LocalFileSystem.cpp
SYS_SRC += $(API_DIR)lp_ticker_api.c
SYS_SRC += $(API_DIR)mbed_interface.c
SYS_SRC += $(API_DIR)pinmap_common.c
SYS_SRC += $(API_DIR)RawSerial.cpp
SYS_SRC += $(API_DIR)retarget.cpp
SYS_SRC += $(API_DIR)rtc_time.c
SYS_SRC += $(API_DIR)semihost_api.c
SYS_SRC += $(API_DIR)SerialBase.cpp
SYS_SRC += $(API_DIR)Serial.cpp
SYS_SRC += $(API_DIR)SPI.cpp
SYS_SRC += $(API_DIR)SPISlave.cpp
SYS_SRC += $(API_DIR)Stream.cpp
SYS_SRC += $(API_DIR)ticker_api.c
SYS_SRC += $(API_DIR)Ticker.cpp
SYS_SRC += $(API_DIR)Timeout.cpp
SYS_SRC += $(API_DIR)Timer.cpp
SYS_SRC += $(API_DIR)TimerEvent.cpp
SYS_SRC += $(API_DIR)us_ticker_api.c
SYS_SRC += $(API_DIR)wait_api.c

HAL_DIR := mbed/targets/hal/TARGET_Freescale/TARGET_KLXX/

SYS_SRC += $(HAL_DIR)analogin_api.c
SYS_SRC += $(HAL_DIR)analogout_api.c
SYS_SRC += $(HAL_DIR)gpio_api.c
SYS_SRC += $(HAL_DIR)i2c_api.c
SYS_SRC += $(HAL_DIR)pinmap.c
SYS_SRC += $(HAL_DIR)port_api.c
SYS_SRC += $(HAL_DIR)pwmout_api.c
SYS_SRC += $(HAL_DIR)rtc_api.c
SYS_SRC += $(HAL_DIR)sleep.c
SYS_SRC += $(HAL_DIR)TARGET_$(TARGET_CPU)/gpio_irq_api.c
SYS_SRC += $(HAL_DIR)TARGET_$(TARGET_CPU)/mbed_overrides.c
SYS_SRC += $(HAL_DIR)TARGET_$(TARGET_CPU)/PeripheralPins.c
SYS_SRC += $(HAL_DIR)TARGET_$(TARGET_CPU)/serial_api.c
SYS_SRC += $(HAL_DIR)TARGET_$(TARGET_CPU)/spi_api.c
SYS_SRC += $(HAL_DIR)us_ticker.c

CMSIS_DIR := mbed/targets/cmsis/TARGET_Freescale/TARGET_KLXX/TARGET_$(TARGET_CPU)/

SYS_SRC += $(CMSIS_DIR)cmsis_nvic.c
SYS_SRC += $(CMSIS_DIR)system_M$(TARGET_CPU)4.c
SYS_SRC += $(CMSIS_DIR)TOOLCHAIN_GCC_ARM/startup_M$(TARGET_CPU)4.S

INCLUDE_PATHS += -I.
INCLUDE_PATHS += -Imillis
INCLUDE_PATHS += -Ipff
INCLUDE_PATHS += -IulibSD
INCLUDE_PATHS += -Imbed
INCLUDE_PATHS += -I$(HAL_DIR)
INCLUDE_PATHS += -I$(HAL_DIR)TARGET_$(TARGET_CPU)/
INCLUDE_PATHS += -I$(CMSIS_DIR)
INCLUDE_PATHS += -Imbed/api
INCLUDE_PATHS += -Imbed/hal
INCLUDE_PATHS += -Imbed/platform
INCLUDE_PATHS += -Imbed/targets/cmsis

LIBRARY_PATHS := -L$(CMSIS_DIR)TOOLCHAIN_GCC_ARM 
# LIBRARIES := -lmbed
LINKER_SCRIPT ?= mbed/targets/cmsis/TARGET_Freescale/TARGET_KLXX/TARGET_$(TARGET_CPU)/TOOLCHAIN_GCC_ARM/M$(TARGET_CPU)4.ld

# Objects and Paths
###############################################################################

include boiler.mk
include flags.mk
