#ifndef __SET_SYS_TIME_H__
#define __SET_SYS_TIME_H__

#include <string>
#include "util/Log.h"

class SetSysTime {
public:
    const char* SET_SYS_TAG = "SET_SYS_TIME";
    bool setSystemTime(long long timeStampMs);
};
