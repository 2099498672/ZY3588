#include "hardware/Net.h"

Net::Net() {
    
}

Net::~Net() {
    // LogInfo(NET_TAG, "Net destroyed");
}

bool Net::getInterfaceIp(const char* interface, char *ip_addr, int addr_len) {
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0); // 创建套接字
    if (fd < 0) {
        LogError(NET_TAG, "%s : creat socket failed. ret = %d", strerror(errno), fd);
        return false;
    }

    memset(&ifr, 0, sizeof(ifr));       // 
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);

    if (ioctl(fd, SIOCGIFADDR, &ifr) == -1) {  /// 获取接口的IP地址
        LogError(NET_TAG, "%s : get interface ip failed.", strerror(errno));
        close(fd);
        return false;
    }

    struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;     // 转换为点分十进制字符串
    inet_ntop(AF_INET, &addr->sin_addr, ip_addr, addr_len);

    close(fd);
    return true;
}