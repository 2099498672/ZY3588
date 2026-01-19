#ifndef __TH_H__
#define __TH_H__

#include <memory>
#include <string>
#include <fstream>
#include <iostream>
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>
#include <fcntl.h>
#include <cmath>
#include <sys/sysinfo.h> 
#include <libudev.h>  
#include <stdio.h>
#include <fcntl.h>

#include "util/Log.h"
class Tf {
public:
    const char* TF_CARD_DEVICE_SIZE_PATH;
    const char* TF_TAG = "TF";
    
    Tf(const char* tfCardDeviceSizePath = "/sys/block/mmcblk1/size");
    float getTfCardSize();

private:

};


#endif  // __TH_H__
