#ifndef __NET_H__
#define __NET_H__

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include "util/Log.h"

class Net {
public:
    const char* NET_TAG = "NET";

    Net();
    ~Net();

    /*
     * @brief   获取网卡ip地址
     * @param   const char* interface: 网卡名称     char *ip_addr：存放网卡地址     int addr_len： 地址长度
     * @return  成功返回false, 失败返回true
     * @example char ip[INET_ADDRSTRLEN];
                board.getInterfaceIp("enp2s0", ip, sizeof(ip));
     */
    virtual bool getInterfaceIp(const char* interface, char *ip_addr, int addr_len);

    // virtual bool 
};

#endif