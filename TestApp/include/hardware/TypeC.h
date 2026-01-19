#ifndef __TYPE_C_H__
#define __TYPE_C_H__

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <sys/inotify.h>

#include "util/Log.h"

class TypeC {
public:
    const char* TYPEC_TAG = "TYPEC";

    /*正插返回 1：   反差返回：0      未插入或异常返回：-1 */
    virtual int typeCTest(std::string typeCPath);

};

#endif