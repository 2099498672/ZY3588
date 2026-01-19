#include "hardware/Rtc.h"

Rtc::Rtc(const std::string& devicePath)
    : rtcDevicePath_(devicePath), rtcFd_(-1) {
}

Rtc::~Rtc() {
    if (rtcFd_ >= 0) {
        close(rtcFd_);
    }
}

bool Rtc::getRtcTime(struct rtc_time& tm) {
    if (ioctl(rtcFd_, RTC_RD_TIME, &tm) == -1) {
        LogError(RTC_TAG, "Failed to read RTC time: %s", strerror(errno));
        close(rtcFd_);
        return false;
    }

    LogDebug(RTC_TAG, "Read RTC Time: %02d:%02d:%02d, Date: %04d-%02d-%02d\n",
           tm.tm_hour, tm.tm_min, tm.tm_sec,
           tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    return true;
}

bool Rtc::setRtcTime(const struct rtc_time& tm) {
    if (ioctl(rtcFd_, RTC_SET_TIME, &tm) == -1) {
        LogError(RTC_TAG, "Failed to set RTC time: %s", strerror(errno));
        return false;
    }

    LogDebug(RTC_TAG, "Set RTC Time: %02d:%02d:%02d, Date: %04d-%02d-%02d\n",
           tm.tm_hour, tm.tm_min, tm.tm_sec,
           tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    return true;
}

bool Rtc::setAndWait(struct rtc_time& tm, int waitSeconds) {
    rtcFd_ = open(rtcDevicePath_.c_str(), O_RDWR);
    if (rtcFd_ < 0) {
        LogError(RTC_TAG, "Failed to open RTC device: %s. ret = %d", rtcDevicePath_.c_str(), rtcFd_);
        rtcFd_ = -1;
    }

    if (!setRtcTime(tm)) {
        close(rtcFd_);
        return false;
    }

    sleep(waitSeconds);

    struct rtc_time newTm;
    if (!getRtcTime(newTm)) {
        close(rtcFd_);
        return false;
    }

    if (newTm.tm_sec != (tm.tm_sec + waitSeconds) % 60) {
        LogError(RTC_TAG, "RTC time did not advance as expected. Expected seconds: %d, Actual seconds: %d",
                 (tm.tm_sec + waitSeconds) % 60, newTm.tm_sec);
        close(rtcFd_);
        return false;
    }

    tm = newTm; // Update the input tm with the new time read from RTC

    LogDebug(RTC_TAG, "RTC Time after waiting %d seconds: %02d:%02d:%02d, Date: %04d-%02d-%02d\n",
           waitSeconds,
           newTm.tm_hour, newTm.tm_min, newTm.tm_sec,
           newTm.tm_year + 1900, newTm.tm_mon + 1, newTm.tm_mday);

    close(rtcFd_);
    return true;
}