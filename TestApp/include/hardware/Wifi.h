#ifndef WIFI_H
#define WIFI_H

#include <cstring>
#include <iostream>
#include <cstdio>
 #include <unistd.h>
 
#include "util/Log.h"

class Wifi {
public:
    Wifi(const char* interface = "wlan0", int scanCount = 5);
    ~Wifi();

    char *scanCommand;
    char *scanResultsCommand;
    const char* INTERFACE;
    int scanCount;
    const char* WIFI_TAG = "WIFI";

    /*
     * @brief  根据传入ssid扫描wifi是否存在该wifi,并获取该wifi的信号强度转化为0-5分制
     * @param  ssid: wifi名称
     * @return  返回信号强度0-5， 失败返回-1
     */
    virtual int wpaScanWifi(const char* ssid);

private:


};



#endif // WIFI_H