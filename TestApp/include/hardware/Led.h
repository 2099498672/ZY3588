#ifndef __LED_H__
#define __LED_H__

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include "util/Log.h"

enum LED_CTRL {
    LED_OFF = 0,
    LED_ON = 1,
};

class Led {

public:

    Led(const char* RLED_BRIGHTNESS_PATH = "/sys/class/leds/led_red/brightness", LED_CTRL rled_status = LED_OFF,
        const char* GLED_BRIGHTNESS_PATH = "/sys/class/leds/led_green/brightness", LED_CTRL gled_status = LED_OFF);

    ~Led();

    bool setRledStatus(LED_CTRL status);
    bool setGledStatus(LED_CTRL status);

    const char* LED_TAG = "LED";
    const char* RLED_BRIGHTNESS_PATH;
    LED_CTRL rled_status;
    const char* GLED_BRIGHTNESS_PATH;
    LED_CTRL gled_status;
    int rledFd;
    int gledFd;
};

#endif  // __LED_H__