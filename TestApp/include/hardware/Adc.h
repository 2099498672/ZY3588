#ifndef __ADC_H__
#define __ADC_H__

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fcntl.h>
#include <unistd.h>

#include <cstring>
#include <cerrno>
#include <iostream>
#include <map>

#include "util/Log.h"

class Adc {
public:
    virtual int get_adc_value(std::string adc_device_path);

    std::map<std::string, std::string> adc_map;


private:    
    const char* ADC_TAG = "ADC";
};

#endif