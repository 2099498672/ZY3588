#ifndef __BASEINFO_H__
#define __BASEINFO_H__

#include <stdio.h>

#include <string>
#include <iostream>

#include "util/Log.h"

class BaseInfo {
public:
    BaseInfo(const char* hwid_path = "/proc/device-tree/hw-board/hw-version");
    virtual bool get_hwid(std::string& hwid);
    virtual std::string get_firmware_version();
    virtual void set_firmware_version(const std::string& firmware_version);
    virtual bool get_app_version(std::string& app_version);

private:
    const char* BASEINFO_TAG = "BASEINFO";
    const char* HWID_PATH;
    std::string firmware_version;
};

#endif // __BASEINFO_H__