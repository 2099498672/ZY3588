#include "hardware/SetSysTime.h"

bool SetSysTime::setSystemTime(long long timeStampMs) {
    struct timeval tv;
    tv.tv_sec = timeStampMs / 1000;
    tv.tv_usec = (timeStampMs % 1000) * 1000;

    if (settimeofday(&tv, nullptr) == 0) {
        LogInfo(SET_SYS_TAG, "System time set successfully");
        return true;
    } else {
        LogError(SET_SYS_TAG, "Failed to set system time");
        return false;
    }
}