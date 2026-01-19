#ifndef __FAN_H__
#define __FAN_H__

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "util/Log.h"

enum FAN_CTRL {
    FAN_OFF = 0,
    FAN_ON = 1,
};


class Fan {
public:
    const char* FAN_CTRL_PATH;
    FAN_CTRL fanStatus;
    const char* FAN_TAG = "FAN";
    int fanFd;

    Fan(const char* ctrlPath = "/sys/class/misc/fan-ctl/pwr_en", FAN_CTRL initialStatus = FAN_OFF);
    ~Fan();
    bool fanCtrl(FAN_CTRL status);
};

#endif // FAN_H