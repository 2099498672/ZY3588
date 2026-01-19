#ifndef __RTC_H__
#define __RTC_H__

#include <memory>
#include <string>
#include <fstream>
#include <iostream>
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>
#include <fcntl.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>

#include "util/Log.h"

class Rtc {

public:
    const std::string rtcDevicePath_;
    int rtcFd_;
    const char* RTC_TAG = "RTC";

    Rtc(const std::string& devicePath = "/dev/rtc0");
    ~Rtc();

    /*
     * @ brief Set the RTC time.
     * @ param tm: The time to set.
     * @ return true if successful, false otherwise.
     */
    virtual bool setRtcTime(const struct rtc_time& tm);

    /*
     * @ brief Get the RTC time.
     * @ param tm: The time to get.
     * @ return true if successful, false otherwise.
     */
    virtual bool getRtcTime(struct rtc_time& tm);

    /*
     * @ brief Set the RTC time and wait for a specified number of seconds.
     * @ param tm: The time to set.
     * @ param waitSeconds: The number of seconds to wait after setting the time.
     * @ return true if successful, false otherwise.
     */
    virtual bool setAndWait(struct rtc_time& tm, int waitSeconds);

private:

};

#endif // __RTC_H__