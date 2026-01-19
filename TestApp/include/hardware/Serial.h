#ifndef __SERIAL_H__
#define __SERIAL_H__

#include <cstring> 
#include <iostream>
#include <string>
#include <random>

#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>
#include <poll.h>

#include "util/Log.h"

class Serial {

public:
    int fd;  
    const char* SERIAL_TAG = "SERIAL";   

    /*
     * @brief 打开并配置串口
     * @param device 串口设备路径，如 "/dev/ttyUSB0" baud_rate 波特率，如 9600, 115200
     * @return 成功返回文件描述符，失败返回 -1
     */
    virtual int openSerial(const char *device, int baudRate);

    /*
     * @brief 串口通信测试
     * @param device 串口设备路径，如 "/dev/ttyUSB0"
     * @param testCount 测试次数
     * @param reCount 重试次数
     * @param reCountTime 重试间隔时间，单位us
     * @return 成功返回 true，失败返回 false
     */
    virtual bool serialTest(const char *device, int reCount = 3, int reCountTimeUs = 1000000, int testCount = 10);


    // 这里是485和CAN的自测方法，不采用
    virtual bool serial485Test(std::vector<std::string>& deviceList, std::vector<bool>& sendResult, std::vector<bool>& recvResult, int testCount = 10);
    virtual bool serial485TestRetry(std::vector<std::string>& deviceList, std::vector<bool>& sendResult, std::vector<bool>& recvResult, 
                                int reCount = 3, int reCountTimeUs = 1000000, int testCount = 10);

    // 这里是485和CAN的自测方法，不采用
    virtual bool serialCanTest(std::vector<std::string>& deviceList, std::vector<bool>& sendResult, std::vector<bool>& recvResult, int testCount = 10);
    virtual bool serialCanTestRetry(std::vector<std::string>& deviceList, std::vector<bool>& sendResult, std::vector<bool>& recvResult, 
                                int reCount = 3, int reCountTimeUs = 1000000, int testCount = 10);

    // CAN 相关方法
    virtual int open_can_device(const char* ifname);
    virtual bool can_send(int fd, uint32_t can_id, const uint8_t* data);
    virtual int can_recv(int fd, unsigned char* buf, int& len, int timeout_ms);
    virtual bool can_test(int fd, int test_count);

    // 485 相关方法
    virtual int open_485_device(const char* device);
    virtual bool serial_485_send(int fd, const unsigned char* buf, size_t len);
    virtual bool serial_485_recv(int fd, unsigned char* buf, size_t len);
    virtual bool serial_485_test(int fd, int test_count);


private:

};

#endif