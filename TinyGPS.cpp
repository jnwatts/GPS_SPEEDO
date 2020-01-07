/*
TinyGPS - a small GPS library for Arduino providing basic NMEA parsing
Based on work by and "distance_to" and "course_to" courtesy of Maarten Lamers.
Suggestion to add satellites(), course_to(), and cardinal(), by Matt Monson.
Precision improvements suggested by Wayne Holder.
Copyright (C) 2008-2013 Mikal Hart
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

/*
 * Additional notes by RBM 2014-03-12
 * **********************************
 *
 * Time accuracy depends on the offset between the UTC edge (usually the PPS)
 * and the transmission of the NMEA messages. If the PPS is not available,
 * which is often the case, then this offset value is unknown, but can
 * typically be a couple of hundred milliseconds, which is further
 * exacerbated by a significant jitter.
 *
 * The first NMEA sentence after the UTC edge is typically $GPRMC, which
 * contains both date and time, so I have modified the code in TinyGPS.cpp
 * to record the millis() when a '$' character is received as the GPS time
 * fix reference, rather than when the time is decoded.
 *
 * -- 2019-01-04 update by JNW
 * Replaced millis() with mbed Timers _gps_time_ref and _gps_position_ref
 */

#include <ctype.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "TinyGPS.h"

#define _GPRMC_TERM   "GPRMC"
#define _GPGGA_TERM   "GPGGA"
#define _GPGSV_TERM   "GPGSV"
#define _GPGSA_TERM   "GPGSA"

TinyGPS::TinyGPS()
  :  _time(GPS_INVALID_TIME)
  ,  _date(GPS_INVALID_DATE)
#ifndef _GPS_TIME_ONLY
  ,  _latitude(GPS_INVALID_ANGLE)
  ,  _longitude(GPS_INVALID_ANGLE)
  ,  _speed(GPS_INVALID_SPEED)
  ,  _course(GPS_INVALID_ANGLE)
  ,  _hdop(GPS_INVALID_HDOP)
#endif /* _GPS_TIME_ONLY */
  ,  _altitude(GPS_INVALID_ALTITUDE)
  ,  _pdop(GPS_INVALID_PDOP)
  ,  _satsinview(GPS_INVALID_SATELLITES)
  ,  _satsused(GPS_INVALID_SATELLITES)
  ,  _fixtype(GPS_INVALID_FIXTYPE)
  ,  _last_time_fix(GPS_INVALID_FIX_TIME)
  ,  _last_position_fix(GPS_INVALID_FIX_TIME)
  ,  _parity(0)
  ,  _is_checksum_term(false)
  ,  _sentence_type(_GPS_SENTENCE_OTHER)
  ,  _term_number(0)
  ,  _term_offset(0)
#ifndef _GPS_NO_STATS
  ,  _encoded_characters(0)
  ,  _good_sentences(0)
  ,  _failed_checksum(0)
#endif
{
  _term[0] = '\0';
  _gps_time_ref.start();
  _gps_position_ref.start();
}

static inline double radians(double degrees) { return M_PI * (degrees / 180); }
static inline double degrees(double radians) { return (radians * 180) / M_PI; }

//
// public methods
//

bool TinyGPS::encode(char c)
{
  bool valid_sentence = false;
  unsigned long charReadTime = _gps_time_ref.read_ms();

#ifndef _GPS_NO_STATS
  ++_encoded_characters;
#endif
  switch(c)
  {
  case ',': // term terminators
    _parity ^= c;
  case '\r':
  case '\n':
  case '*':
    if (_term_offset < sizeof(_term))
    {
      _term[_term_offset] = 0;
      valid_sentence = term_complete();
    }
    ++_term_number;
    _term_offset = 0;
    _is_checksum_term = c == '*';
    //printf("TinyGPS::encode(): About to return %s\n", valid_sentence ? "true" : "false");
    break;

  case '$': // sentence begin
    _term_number = _term_offset = 0;
    _parity = 0;
    _sentence_type = _GPS_SENTENCE_OTHER;
    _is_checksum_term = false;
    _new_time_fix = charReadTime;   // synch time fix age at start of sentence
    break;

  default:
    // ordinary characters
    if (_term_offset < sizeof(_term) - 1)
      _term[_term_offset++] = c;
    if (!_is_checksum_term)
      _parity ^= c;
    break;
  }

  return valid_sentence;
}

#ifndef _GPS_NO_STATS
void TinyGPS::stats(unsigned long *chars, unsigned short *sentences, unsigned short *failed_cs)
{
  if (chars) *chars = _encoded_characters;
  if (sentences) *sentences = _good_sentences;
  if (failed_cs) *failed_cs = _failed_checksum;
}
#endif

//
// internal utilities
//
int TinyGPS::from_hex(char a)
{
  if (a >= 'A' && a <= 'F')
    return a - 'A' + 10;
  else if (a >= 'a' && a <= 'f')
    return a - 'a' + 10;
  else
    return a - '0';
}

unsigned long TinyGPS::parse_decimal()
{
  char *p = _term;
  bool isneg = *p == '-';
  if (isneg) ++p;
  unsigned long ret = 100UL * atol(p);
  while (gpsisdigit(*p)) ++p;
  if (*p == '.')
  {
    if (gpsisdigit(p[1]))
    {
      ret += 10 * (p[1] - '0');
      if (gpsisdigit(p[2]))
        ret += p[2] - '0';
    }
  }
  return isneg ? -ret : ret;
}

// Parse a string in the form ddmm.mmmmmmm...
unsigned long TinyGPS::parse_degrees()
{
  char *p;
  unsigned long left_of_decimal = atol(_term);
  unsigned long hundred1000ths_of_minute = (left_of_decimal % 100UL) * 100000UL;
  for (p=_term; gpsisdigit(*p); ++p);
  if (*p == '.')
  {
    unsigned long mult = 10000;
    while (gpsisdigit(*++p))
    {
      hundred1000ths_of_minute += mult * (*p - '0');
      mult /= 10;
    }
  }
  return (left_of_decimal / 100) * 1000000 + (hundred1000ths_of_minute + 3) / 6;
}

#define COMBINE(sentence_type, term_number) (((unsigned)(sentence_type) << 5) | term_number)

// Processes a just-completed term
// Returns true if new sentence has just passed checksum test and is validated
bool TinyGPS::term_complete()
{
  if (_is_checksum_term)
  {
    uint8_t checksum = 16 * from_hex(_term[0]) + from_hex(_term[1]);
    //printf("TinyGPS::term_complete(): inside \"if (_is_checksum_term).  checksum=0x%02x  _parity=0x%02x\"\n", checksum, _parity);
    if (checksum == _parity)
    {
#ifndef _GPS_NO_STATS
      ++_good_sentences;
#endif

      switch(_sentence_type)
      {
      case _GPS_SENTENCE_GPRMC:
        _time      = _new_time;
        _date      = _new_date;
        _last_time_fix = _new_time_fix;
        if (_gps_data_good)
        {
            _last_position_fix = _new_position_fix;
        }
#ifndef _GPS_TIME_ONLY
        _latitude  = _new_latitude;
        _longitude = _new_longitude;
        _speed     = _new_speed;
        _course    = _new_course;
#endif /* _GPS_TIME_ONLY */
        // printf("TinyGPS::term_complete(): inside \"case _GPS_SENTENCE_GPRMC\".\n");
        break;
      case _GPS_SENTENCE_GPGGA:
        if (_gps_data_good)
        {
            _last_position_fix = _new_position_fix;
        }
        _altitude  = _new_altitude;     _new_altitude   = 0;
        _time      = _new_time;
#ifndef _GPS_TIME_ONLY
        _latitude  = _new_latitude;
        _longitude = _new_longitude;
        _hdop      = _new_hdop;         _new_hdop       = 0;
#endif /* _GPS_TIME_ONLY */
        _satsused  = _new_satsused;     _new_satsused   = 0;
        break;
      case _GPS_SENTENCE_GPGSV:
        _satsinview = _new_satsinview;  _new_satsinview = 0;
        break;
      case _GPS_SENTENCE_GPGSA:
        _fixtype = _new_fixtype;        _new_fixtype    = 0;
        _pdop    = _new_pdop;           _new_pdop       = 0;
        break;
      }

      return true;
    }

#ifndef _GPS_NO_STATS
    else
      ++_failed_checksum;
#endif
    return false;
  }

  // the first term determines the sentence type
  if (_term_number == 0)
  {
    if (strncmp(_term, _GPRMC_TERM, 5) == 0)
      _sentence_type = _GPS_SENTENCE_GPRMC;
    else if (strncmp(_term, _GPGGA_TERM, 5) == 0)
      _sentence_type = _GPS_SENTENCE_GPGGA;
    else if (strncmp(_term, _GPGSV_TERM, 5) == 0)
      _sentence_type = _GPS_SENTENCE_GPGSV;
    else if (strncmp(_term, _GPGSA_TERM, 5) == 0)
      _sentence_type = _GPS_SENTENCE_GPGSA;
    else
      _sentence_type = _GPS_SENTENCE_OTHER;
    return false;
  }

  // Finish here if the term is empty or it's a sentence type that we ignore
  if (_term[0] == '\0' || _sentence_type == _GPS_SENTENCE_OTHER)
    return false;

  switch(COMBINE(_sentence_type, _term_number))
  {
  case COMBINE(_GPS_SENTENCE_GPRMC, 1): // Time
    _new_time = parse_decimal();
    break;
  case COMBINE(_GPS_SENTENCE_GPGGA, 1): // Time
    // Ignore this time because it's already skewed by > 100mS
    break;
  case COMBINE(_GPS_SENTENCE_GPRMC, 2): // GPRMC validity
    _gps_data_good = _term[0] == 'A';
    break;
  case COMBINE(_GPS_SENTENCE_GPGSA, 2): // Fix type
    _new_fixtype = (unsigned char) atol(_term);
    break;
  case COMBINE(_GPS_SENTENCE_GPGSA, 15): // PDOP (position dilution of precision)
      _new_pdop = (unsigned short)parse_decimal();
    break;
  case COMBINE(_GPS_SENTENCE_GPRMC, 3): // Latitude
  case COMBINE(_GPS_SENTENCE_GPGGA, 2):
#ifndef _GPS_TIME_ONLY
    _new_latitude = parse_degrees();
#endif /* _GPS_TIME_ONLY */
    _new_position_fix = _gps_position_ref.read_ms();
    break;
  case COMBINE(_GPS_SENTENCE_GPGSV, 3): // Satellites in view
    // we've got our number of sats
    // NOTE: we will more than likely hit this a few times in a row, because
    // there are usually multiple GPGSV sentences to describe all of the
    // satelites, but that's OK because the each contain the total number
    // of satellites in view.
    _new_satsinview = (unsigned char) atol(_term);
    break;
  case COMBINE(_GPS_SENTENCE_GPRMC, 4): // N/S
  case COMBINE(_GPS_SENTENCE_GPGGA, 3):
#ifndef _GPS_TIME_ONLY
    if (_term[0] == 'S')
      _new_latitude = -_new_latitude;
#endif /* _GPS_TIME_ONLY */
    break;
  case COMBINE(_GPS_SENTENCE_GPRMC, 5): // Longitude
  case COMBINE(_GPS_SENTENCE_GPGGA, 4):
#ifndef _GPS_TIME_ONLY
    _new_longitude = parse_degrees();
#endif /* _GPS_TIME_ONLY */
    break;
  case COMBINE(_GPS_SENTENCE_GPRMC, 6): // E/W
  case COMBINE(_GPS_SENTENCE_GPGGA, 5):
#ifndef _GPS_TIME_ONLY
    if (_term[0] == 'W')
      _new_longitude = -_new_longitude;
#endif /* _GPS_TIME_ONLY */
    break;
  case COMBINE(_GPS_SENTENCE_GPRMC, 7): // Speed (GPRMC)
#ifndef _GPS_TIME_ONLY
    _new_speed = parse_decimal();
#endif /* _GPS_TIME_ONLY */
    break;
  case COMBINE(_GPS_SENTENCE_GPRMC, 8): // Course (GPRMC)
#ifndef _GPS_TIME_ONLY
    _new_course = parse_decimal();
#endif /* _GPS_TIME_ONLY */
    break;
  case COMBINE(_GPS_SENTENCE_GPRMC, 9): // Date (GPRMC)
    _new_date = atol(_term);
    break;
  case COMBINE(_GPS_SENTENCE_GPGGA, 6): // Fix data (GPGGA)
    _gps_data_good = _term[0] > '0';
    break;
  case COMBINE(_GPS_SENTENCE_GPGGA, 7): // Satellites used
    _new_satsused = (unsigned char) atol(_term);
    break;
  case COMBINE(_GPS_SENTENCE_GPGGA, 8): // HDOP
#ifndef _GPS_TIME_ONLY
    _new_hdop = parse_decimal();
#endif /* _GPS_TIME_ONLY */
    break;
  case COMBINE(_GPS_SENTENCE_GPGGA, 9): // Altitude (GPGGA)
    _new_altitude = parse_decimal();
    break;
  }
  return false;
}

/* static */
double TinyGPS::distance_between (double lat1, double long1, double lat2, double long2)
{
  // returns distance in meters between two positions, both specified
  // as signed decimal-degrees latitude and longitude. Uses great-circle
  // distance computation for hypothetical sphere of radius 6372795 meters.
  // Because Earth is no exact sphere, rounding errors may be up to 0.5%.
  // Courtesy of Maarten Lamers
  double delta = radians(long1-long2);
  double sdlong = sin(delta);
  double cdlong = cos(delta);
  lat1 = radians(lat1);
  lat2 = radians(lat2);
  double slat1 = sin(lat1);
  double clat1 = cos(lat1);
  double slat2 = sin(lat2);
  double clat2 = cos(lat2);
  delta = (clat1 * slat2) - (slat1 * clat2 * cdlong);
  delta *= delta;
  delta += (clat2 * sdlong) * (clat2 * sdlong);
  delta = sqrt(delta);
  double denom = (slat1 * slat2) + (clat1 * clat2 * cdlong);
  delta = atan2(delta, denom);
  return delta * 6372795;
}

double TinyGPS::course_to (double lat1, double long1, double lat2, double long2)
{
  // returns course in degrees (North=0, West=270) from position 1 to position 2,
  // both specified as signed decimal-degrees latitude and longitude.
  // Because Earth is no exact sphere, calculated course may be off by a tiny fraction.
  // Courtesy of Maarten Lamers
  double dlon = radians(long2-long1);
  lat1 = radians(lat1);
  lat2 = radians(lat2);
  double a1 = sin(dlon) * cos(lat2);
  double a2 = sin(lat1) * cos(lat2) * cos(dlon);
  a2 = cos(lat1) * sin(lat2) - a2;
  a2 = atan2(a1, a2);
  if (a2 < 0.0)
  {
    a2 += (M_PI * 2);
  }
  return degrees(a2);
}

const char *TinyGPS::cardinal (double course)
{
  static const char* directions[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};

  int direction = (int)((course + 11.25f) / 22.5f);
  return directions[direction % 16];
}

// lat/long in MILLIONTHs of a degree and age of fix in milliseconds
// (note: versions 12 and earlier gave this value in 100,000ths of a degree.
void TinyGPS::get_position(long *latitude, long *longitude, unsigned long *fix_age)
{
#ifndef _GPS_TIME_ONLY
  if (latitude)  *latitude  = _latitude;
  if (longitude) *longitude = _longitude;
#else
  if (latitude)  *latitude  = GPS_INVALID_ANGLE;
  if (longitude) *longitude = GPS_INVALID_ANGLE;
#endif /* _GPS_TIME_ONLY */

  if (fix_age)
  {
    *fix_age = _last_position_fix == GPS_INVALID_FIX_TIME
                 ? GPS_INVALID_AGE : (_gps_position_ref.read_ms() - _last_position_fix);
  }
}

// date as ddmmyy, time as hhmmsscc, and age in milliseconds
void TinyGPS::get_datetime(unsigned long *date, unsigned long *time, unsigned long *age)
{
  if (date) *date = _date;
  if (time) *time = _time;
  if (age) *age = _last_time_fix == GPS_INVALID_FIX_TIME ?
   GPS_INVALID_AGE : _gps_time_ref.read_ms() - _last_time_fix;
}

void TinyGPS::d_get_position(double *latitude, double *longitude, unsigned long *fix_age)
{
  long lat, lon;
  get_position(&lat, &lon, fix_age);
  *latitude = lat == GPS_INVALID_ANGLE ? GPS_INVALID_F_ANGLE : (lat / 1000000.0);
  *longitude = lat == GPS_INVALID_ANGLE ? GPS_INVALID_F_ANGLE : (lon / 1000000.0);
}

void TinyGPS::crack_datetime(int *year, uint8_t *month, uint8_t *day,
  uint8_t *hour, uint8_t *minute, uint8_t *second, uint8_t *hundredths, unsigned long *age)
{
  unsigned long date, time;
  uint16_t t1, t2;              // Used to hold temporary values during calculation
  get_datetime(&date, &time, age);

  // decompose date "ddmmyy"
  t1 = date / 100;            // t1 contains ddmm
  if (year)
  {
      t2 = date - (t1 * 100); // t2 contains yy
      *year = t2;
      *year += *year > 80 ? 1900 : 2000;
  }

  t2 = t1 / 100;              // t2 contains dd
  if (month) *month = t1 - (t2 * 100);
  if (day) *day = t2;

  // Decompose time "hhmmsscc"
  // Reuse the "date variable because we need a long here
  date = time / 100;          // date contains hhmmss
  if (hundredths) *hundredths = time - (date * 100);

  t1 = date  / 100;           // t1 contains hhmm
  if (second) *second = date - (t1 * 100);

  t2 = t1 / 100;              // t2 contains hh
  if (hour) *hour = t2;
  if (minute) *minute = t1 - (t2 * 100);
}

double TinyGPS::d_altitude()
{
  return _altitude == GPS_INVALID_ALTITUDE ? GPS_INVALID_F_ALTITUDE : _altitude / 100.0;
}

double TinyGPS::d_course()
{
#ifndef _GPS_TIME_ONLY
  return _course == GPS_INVALID_ANGLE ? GPS_INVALID_F_ANGLE : _course / 100.0;
#else
  return GPS_INVALID_F_ANGLE;
#endif /* _GPS_TIME_ONLY */
}

double TinyGPS::d_speed_knots()
{
#ifndef _GPS_TIME_ONLY
  return _speed == GPS_INVALID_SPEED ? GPS_INVALID_F_SPEED : _speed / 100.0;
#else
  return GPS_INVALID_F_SPEED;
#endif /* _GPS_TIME_ONLY */
}

double TinyGPS::d_speed_mph()
{
#ifndef _GPS_TIME_ONLY
  double sk = d_speed_knots();
  return sk == GPS_INVALID_F_SPEED ? GPS_INVALID_F_SPEED : _GPS_MPH_PER_KNOT * sk;
#else
  return GPS_INVALID_F_SPEED;
#endif /* _GPS_TIME_ONLY */
}

double TinyGPS::d_speed_mps()
{
#ifndef _GPS_TIME_ONLY
  double sk = d_speed_knots();
  return sk == GPS_INVALID_F_SPEED ? GPS_INVALID_F_SPEED : _GPS_MPS_PER_KNOT * sk;
#else
  return GPS_INVALID_F_SPEED;
#endif /* _GPS_TIME_ONLY */
}

double TinyGPS::d_speed_kmph()
{
#ifndef _GPS_TIME_ONLY
  double sk = d_speed_knots();
  return sk == GPS_INVALID_F_SPEED ? GPS_INVALID_F_SPEED : _GPS_KMPH_PER_KNOT * sk;
#else
  return GPS_INVALID_F_SPEED;
#endif /* _GPS_TIME_ONLY */
}

const double TinyGPS::GPS_INVALID_F_ANGLE = 1000.0;
const double TinyGPS::GPS_INVALID_F_ALTITUDE = 1000000.0;
const double TinyGPS::GPS_INVALID_F_SPEED = -1.0;
