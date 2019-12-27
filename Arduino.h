#pragma once

#define ARDUINO 100

#include "mbed.h"
#include <time.h>
#include <millis.h>
#include <stdint.h>
#include <math.h>

#define TWO_PI M_PI * 2
#define HIGH 1
#define LOW 0


#define delayMicroseconds(x) wait_us(x)


inline float radians(float degrees) { return M_PI * (degrees / 180); }
inline float degrees(float radians) { return (radians * 180) / M_PI; }
inline float sq(float f)            { return f * f; }

typedef unsigned char byte;

