#ifndef __GET_DP_LANES_H__
#define __GET_DP_LANES_H__

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
#include "util/Log.h"

class GetDpLanes {
public:
    const char* GET_DP_LANES_TAG = "GET_DP_LANES";

    virtual int readDpLanes(const char* dpLanesPath = "/sys/devices/platform/fed80000.phy/dp_lanes");

};

#endif // __GET_DP_LANES_H__
