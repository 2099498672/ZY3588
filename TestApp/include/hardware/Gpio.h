#ifndef __GPIO_H__
#define __GPIO_H__

#include <memory>
#include <string>
#include <fstream>
#include <iostream>
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>
#include <fcntl.h>

#include "util/Log.h"

class Gpio {
private:
    const char* GPIO_TAG = "Gpio";

public:
    /*
     * @brief   根据gpio编号导出gpio
     * @param   int gpioNum: gpio编号
     * @return  导出成功返回true, 导出失败返回false
     */
    virtual bool gpioExport(int gpioNum);

    /*
     * @brief   函数重载，根据gpio编号导出gpio
     * @param   const char* GPIO_NUM: gpio编号
     * @return  导出成功返回true, 导出失败返回false
     */
    virtual bool gpioExport(const char* GPIO_NUM);

    /*
     * @brief   设置gpio方向，前提是已导出gpio
     * @param   const char* GPIO_PATH: gpio路径     const char* DIR：in(输入)，out(输出)
     * @return  设置成功返会true, 设置失败返回false
     */
    virtual bool gpioSetDir(const char* GPIO_PATH, const char* DIR);

    /*
     * @brief   获取gpio方向，前提是已导出gpio
     * @param   const char* GPIO_PATH: gpio路径     
     * @param   std::string& dir： 用于存放获取到的方向，in(输入)，out(输出)
     * @return  获取成功返会true, 获取失败返回false。
     */
    virtual bool gpioGetDir(const char* GPIO_PATH, std::string& dir);

    /*
     * @brief   设置gpio输出值
     * @param   int gpioNum: gpio 编号      int value： 要设置的值
     * @return  设置成功返会true, 设置失败返回false
     */
    virtual bool gpioSetValue(int gpioNum, int value);

    /*
     * @brief   获取gpio输入值
     * @param   int gpioNum: gpio 编号, 判断输入值放入value
     * @return  和value相等返回true, 否则返回false，失败返回false
     */
    virtual bool gpioGetValue(int gpioNum, int value);

    /*
     * @brief   获取gpio输入值
     * @param   int gpioNum: gpio 编号
     * @return  获取成功返回输入值，失败返回-1
     */
    // virtual int gpioGetValue(int gpioNum);
};

#endif