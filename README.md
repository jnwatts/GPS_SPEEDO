# GPS_SPEEDO

It's the middle of winter and I'm too cheap & lazy to fix my vehicle's speedo cable, so I built this up from some Kinetis FRDM boards I had plus a few parts from DFRobot:

* https://wiki.dfrobot.com/LED_Keypad_Shield_V1.0_SKU_DFR0382
* https://wiki.dfrobot.com/GPS_Module_With_Enclosure__SKU_TEL0094_

The end result is surprising suitable for use away from (very) tall buildings and tunnels. (I've seen my speed jump by \~4mph while passing under small bridges)

# Licenses & distribution

Most of the libraries used by this project are licensed by Apache, MIT or other manufacturer specific licenses. Notable exception are:

* GPL-2.0-only OR LGPL-2.1-only: Roland Riegel's [sd-reader](http://www.roland-riegel.de/sd-reader/) library, which I've modified to remove AVR-specific code.
* LGPL-2.1-or-later: Mikal Hart's [TinyGPS](https://github.com/mikalhart/TinyGPS), which I've modified to replace Arduino-specific code with mbed-specific mechanisms.

I've licensed my own new work on a per-file basis as I saw fit. Files which are intrinsically tied to other LGPL are licensed by LGPL-2.1-or-later, while most others are my preferred BSD-2-Clause.
