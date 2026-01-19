#include "hardware/Serial.h"
#include <vector>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <errno.h>
int Serial::openSerial(const char *device, int baudRate) {
    struct termios options;

    fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        LogError(SERIAL_TAG, "%s : open %s failed. fd = %d", strerror(errno), device, fd);
        return -1;
    }

    fcntl(fd, F_SETFL, 0);                   // 配置为阻塞模式

    if (tcgetattr(fd, &options) < 0) {       // 获取当前配置
        LogError(SERIAL_TAG, "%s : open %s failed. fd = %d", strerror(errno), device, fd);
        close(fd);
        return -1;
    }

    speed_t speed;                            // 设置波特率
    switch (baudRate) {
        case 9600:   speed = B9600; break;
        case 19200:  speed = B19200; break;
        case 38400:  speed = B38400; break;
        case 57600:  speed = B57600; break;
        case 115200: speed = B115200; break;
        default:
            LogError(SERIAL_TAG, "no support baud rate: %d\n", baudRate);
            close(fd);
            return -1;
    }

    cfsetispeed(&options, speed);
    cfsetospeed(&options, speed);

    options.c_cflag &= ~PARENB;    // 无奇偶校验
    options.c_cflag &= ~CSTOPB;    // 1位停止位
    options.c_cflag &= ~CSIZE;     // 清除数据位设置
    options.c_cflag |= CS8;        // 8位数据位
    options.c_cflag &= ~CRTSCTS;   // 禁用硬件流控制
    options.c_cflag |= CREAD | CLOCAL;  // 启用接收器, 忽略调制解调器控制线
    options.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL | INLCR | IGNCR);     // 禁用软件流控制和字符转换
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);       // 设置为原始模式
    options.c_oflag &= ~OPOST;

    options.c_cc[VTIME] = 10;  // 读取超时 (1秒)       
    options.c_cc[VMIN] = 0;    // 最小读取字符数

    if (tcsetattr(fd, TCSANOW, &options) < 0) {        // 应用配置
        LogError(SERIAL_TAG, "%s set serial config failed. ", baudRate);
        close(fd);
        return -1;
    }

    tcflush(fd, TCIOFLUSH);       // 清空缓冲区

    return fd;
}

// 串口自测方法
bool Serial::serialTest(const char *device, int reCount, int reCountTimeUs, int testCount) {
    bool result = true;
    const char *testData = "1234567890!@#$%^&*()";
    char buffer[256];
    ssize_t bytesWritten, bytesRead;

    for (int attempt = 0; attempt <= reCount; attempt++) {   // 重试机制
        result = true;   // 每次重试开始时假设成功

        int fd = openSerial(device, 115200);
        if (fd < 0) {
            LogError(SERIAL_TAG, "open serial %s failed on attempt %d", device, attempt + 1);
            result = false;
            usleep(reCountTimeUs);   // 等待一段时间后重试
            continue;
        }

        for (int i = 0; i < testCount; i++) {    // 跳过前10%的数据，后80%数据用于测试
            bytesWritten = -1;
            bytesRead = -1;
            memset(buffer, 0, sizeof(buffer));
            bytesWritten = write(fd, testData, strlen(testData));      // 发送数据
            if (bytesWritten < 0) {
                LogError(SERIAL_TAG, "write failed. ret = %d", bytesWritten);
                result = false;        // 写入失败, 标记失败
                break;                 // 退出当前测试循环，进行重试
            }

            usleep(100000);  // 等待数据回传100ms 

            bytesRead = read(fd, buffer, sizeof(buffer) - 1);   // 读取数据
            if (bytesRead < 0) {
                LogError(SERIAL_TAG, "read failed. ret = %d", bytesRead);
                result = false;       // 读取失败
                break;                // 退出当前测试循环，进行重试
            }
            buffer[bytesRead] = '\0';
            LogDebug(SERIAL_TAG, "read : %s", buffer);
            if ((bytesRead != bytesWritten || strcmp(buffer, testData) != 0)) {      // 
                LogError(SERIAL_TAG, "test failed. written = %d, read = %d", bytesWritten, bytesRead);
                result = false;
                break;                // 退出当前测试循环，进行重试
            } 
        }

        close(fd);   // 关闭串口
        
        if (result == true) {
            LogDebug(SERIAL_TAG, "serial test success on attempt %d", attempt + 1);
            break;  // 测试成功，退出重试循环
        } else {
            LogDebug(SERIAL_TAG, "serial test failed on attempt %d, retrying...", attempt + 1);
            usleep(reCountTimeUs);   // 等待一段时间后重试
        }
    }

    LogDebug(SERIAL_TAG, "test ok : %d", result);
    return result;
}

bool Serial::serial485Test(std::vector<std::string>& deviceList, std::vector<bool>& sendResult, std::vector<bool>& recvResult, int testCount) {
    if (deviceList.size() < 2) {                     // 485 测试，至少需要两个设备
        LogError(SERIAL_TAG, "deviceList size < 2");
        return false;
    }
    std::vector<int> fds(deviceList.size(), -1);    // 存储每个设备的文件描述符
    sendResult.resize(deviceList.size(), true);     // 初始化发送结果, 默认成功
    recvResult.resize(deviceList.size(), true);     // 初始化接收结果, 默认成功

    for (int i = 0; i < deviceList.size(); i++) {   // 打开所有设备
        if ((fds[i] = openSerial(deviceList[i].c_str(), 115200)) < 0) {
            sendResult[i] = false;                  // 打开失败，标记发送失败
            recvResult[i] = false;                  // 打开失败，标记接收失败
            fds[i] = -1;                            // 确保文件描述符为 -1
            LogError(SERIAL_TAG, "open %s failed", deviceList[i].c_str());
        }
    }

    const char *testData = "1234567890!@#$%^&*()";
    char buffer[256];

    for (int i = 0; i < deviceList.size(); i++) {       // 每个设备依次发送数据
        if (fds[i] == -1) continue;                     // 跳过打开失败的设备
        for (int j = 0; j < deviceList.size(); j++) {   // 其他设备接收数据
            if (j == i || fds[j] == -1) continue;       // 跳过自己和打开失败的设备
     
            std::cout << "Device " << deviceList[i] << " sent data to Device " << deviceList[j] << std::endl;

            for(int k = 0; k < testCount; k++) {
                ssize_t bytesWritten = -1;
                ssize_t bytesRead = -1;
                bytesWritten = write(fds[i], testData, strlen(testData));     // i发送数据
                if (bytesWritten < 0) {
                    LogError(SERIAL_TAG, "write to %s failed. ret = %d", deviceList[i].c_str(), bytesWritten);
                    sendResult[i] = false;              // 标记发送失败
                    continue;                           // 尝试下一次发送
                }
                usleep(10000);  

                memset(buffer, 0, sizeof(buffer));
                bytesRead = read(fds[j], buffer, sizeof(buffer) - 1);        // j接收数据
                printf("bytesRead = %s  readCount = %d\n", buffer, bytesRead);
                if (bytesRead < 0) {
                    LogError(SERIAL_TAG, "read from %s failed. ret = %d", deviceList[j].c_str(), bytesRead);
                    recvResult[j] = false;          // 标记接收失败，只要有一次失败就标记
                    break;                          // 下一个设备
                } else if (bytesRead > 0) {
                    buffer[bytesRead] = '\0';
                    LogDebug(SERIAL_TAG, "read from %s: %s", deviceList[j].c_str(), buffer);
                    if (bytesRead == bytesWritten && strcmp(buffer, testData) == 0) {
                        continue;                   // 成功接收到正确数据， testCount次数全部成功才算成功
                    } else {
                        recvResult[j] = false;       // 标记接收失败， 只要有一次失败就标记
                        LogError(SERIAL_TAG, "invalid response for data sent from %s to %s", deviceList[i].c_str(), deviceList[j].c_str());
                        break;                       // 下一个设备
                    }
                } else {
                    recvResult[j] = false;          // 标记接收失败，只要有一次失败就标记
                    LogError(SERIAL_TAG, "no data received from %s", deviceList[j].c_str());
                    break;                          // 下一个设备
                }
            }
        }
    }
    // 关闭所有打开的设备
    for (int i = 0; i < fds.size(); i++) {
        if (fds[i] != -1) {
            close(fds[i]);
            fds[i] = -1;
        }
    }
    return true;
}

bool Serial::serial485TestRetry(std::vector<std::string>& deviceList, std::vector<bool>& sendResult, std::vector<bool>& recvResult, int reCount, int reCountTimeUs, int testCount) {
    if (deviceList.size() < 2) {                     // 485 测试，至少需要两个设备
        LogError(SERIAL_TAG, "deviceList size < 2");
        return false;
    }

    const char *testData = "1234567890!@#$%^&*()";
    char buffer[256];

    for (int attempt = 0; attempt <= reCount; attempt++) {   // 重试机制
        std::vector<int> fds(deviceList.size(), -1);    // 存储每个设备的文件描述符
        sendResult.resize(deviceList.size(), true);     // 初始化发送结果, 默认成功
        recvResult.resize(deviceList.size(), true);     // 初始化接收结果, 默认成功

        for (int i = 0; i < deviceList.size(); i++) {   // 重置结果
            sendResult[i] = true;
            recvResult[i] = true;
        }

        // std::cout << "测试结果置位：发送全部成功，接收全部成功" << std::endl;
        log_thread_safe(LOG_LEVEL_INFO, SERIAL_TAG, "test results reset: all send success, all recv success(default)");
        // 打印结果置位
        for (int i = 0; i < deviceList.size(); i++) {
            std::cout << "Device " << deviceList[i] << ": Send Result = " 
                      << (sendResult[i] ? "Success" : "Failed") 
                      << ", Recv Result = " 
                      << (recvResult[i] ? "Success" : "Failed") 
                      << std::endl;
        }

        for (int i = 0; i < deviceList.size(); i++) {   // 打开所有设备
            if ((fds[i] = openSerial(deviceList[i].c_str(), 115200)) < 0) {
                sendResult[i] = false;                  // 打开失败，标记发送失败
                recvResult[i] = false;                  // 打开失败，标记接收失败
                fds[i] = -1;                            // 确保文件描述符为 -1
                LogError(SERIAL_TAG, "open %s failed", deviceList[i].c_str()); 
            }
        }

        for (int i = 0; i < deviceList.size(); i++) {       // 每个设备依次发送数据
            if (fds[i] == -1) continue;                     // 跳过打开失败的设备
            for (int j = 0; j < deviceList.size(); j++) {   // 其他设备接收数据
                if (j == i || fds[j] == -1) continue;       // 跳过自己和打开失败的设备
        
                std::cout << "Device " << deviceList[i] << " sent data to Device " << deviceList[j] << std::endl;

                for(int k = 0; k < testCount; k++) {
                    ssize_t bytesWritten = -1;
                    ssize_t bytesRead = -1;
                    bytesWritten = write(fds[i], testData, strlen(testData));     // i发送数据
                    if (bytesWritten < 0) {
                        LogError(SERIAL_TAG, "write to %s failed. ret = %d", deviceList[i].c_str(), bytesWritten);
                        sendResult[i] = false;              // 标记发送失败
                        continue;                           // 尝试下一次发送
                    }
                    usleep(100000);  

                    memset(buffer, 0, sizeof(buffer));
                    bytesRead = read(fds[j], buffer, sizeof(buffer) - 1);        // j接收数据
                    printf("bytesRead = %s  readCount = %d\n", buffer, bytesRead);
                    if (bytesRead < 0) {
                        LogError(SERIAL_TAG, "read from %s failed. ret = %d", deviceList[j].c_str(), bytesRead);
                        recvResult[j] = false;          // 标记接收失败，只要有一次失败就标记
                        break;                          // 下一个设备
                    } else if (bytesRead > 0) {
                        buffer[bytesRead] = '\0';
                        LogDebug(SERIAL_TAG, "read from %s: %s", deviceList[j].c_str(), buffer);
                        if (bytesRead == bytesWritten && strcmp(buffer, testData) == 0) {
                            continue;                   // 成功接收到正确数据， testCount次数全部成功才算成功
                        } else {
                            recvResult[j] = false;       // 标记接收失败， 只要有一次失败就标记
                            LogError(SERIAL_TAG, "invalid response for data sent from %s to %s", deviceList[i].c_str(), deviceList[j].c_str());
                            break;                       // 下一个设备
                        }
                    } else {
                        recvResult[j] = false;          // 标记接收失败，只要有一次失败就标记
                        LogError(SERIAL_TAG, "no data received from %s", deviceList[j].c_str());
                        break;                          // 下一个设备
                    }
                }
            }
        }
                // 关闭所有打开的设备
        for (int i = 0; i < fds.size(); i++) {
            if (fds[i] != -1) {
                close(fds[i]);
                fds[i] = -1;
            }
        }

        // 打印本次测试结果
        std::cout << "本次测试结果：" << std::endl;
        for (int i = 0; i < deviceList.size(); i++) {
            std::cout << "Device " << deviceList[i] << ": Send Result = " 
                      << (sendResult[i] ? "Success" : "Failed") 
                      << ", Recv Result = " 
                      << (recvResult[i] ? "Success" : "Failed") 
                      << std::endl;
        }

        for (int i = 0; i < deviceList.size(); i++) {
            if (sendResult[i] == false || recvResult[i] == false) {
                LogDebug(SERIAL_TAG, "serial 485 test failed on attempt %d, retrying...", attempt + 1);
                usleep(reCountTimeUs);   // 等待一段时间后重试
                break;
            }
            if (i == deviceList.size() - 1) {
                LogDebug(SERIAL_TAG, "serial 485 test success on attempt %d", attempt + 1);
                return true;  // 测试成功，退出重试循环
            }
        }
    }

    return true;
}



bool Serial::serialCanTest(std::vector<std::string>& deviceList, std::vector<bool>& sendResult, std::vector<bool>& recvResult, int testCount) {
    if (deviceList.size() < 2) {                     // CAN 测试，至少需要两个设备
        LogError(SERIAL_TAG, "deviceList size < 2");
        return false;
    }
    
    std::vector<int> sockfds(deviceList.size(), -1); // 存储每个设备的套接字描述符
    sendResult.resize(deviceList.size(), true);      // 初始化发送结果, 默认成功
    recvResult.resize(deviceList.size(), true);      // 初始化接收结果, 默认成功

    // 打开所有CAN设备
    for (int i = 0; i < deviceList.size(); i++) {
        //ip link set down can0 
        //ip link set can0 type can bitrate 250000
        //ip link set up can0
        int ret = system(("ip link set down " + deviceList[i]).c_str());    // 无法使用系统命令设置时，收发结果置位false, system命令不靠谱，后期需要改为ioctl方式
        if (ret != 0) {
            sendResult[i] = false;
            recvResult[i] = false;
            LogError(SERIAL_TAG, "set down %s failed", deviceList[i].c_str());
        }
        ret = system(("ip link set " + deviceList[i] + " type can bitrate 250000").c_str());
        if (ret != 0) {
            sendResult[i] = false;
            recvResult[i] = false;
            LogError(SERIAL_TAG, "set bitrate for %s failed", deviceList[i].c_str());
        }

        system(("ip link set up " + deviceList[i]).c_str());
        if (ret != 0) {
            sendResult[i] = false;
            recvResult[i] = false;
            LogError(SERIAL_TAG, "set up %s failed", deviceList[i].c_str());
        }

        int sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
        if (sockfd < 0) {
            LogError(SERIAL_TAG, "create CAN socket failed for %s", deviceList[i].c_str());
            sendResult[i] = false;
            recvResult[i] = false;
            sockfds[i] = -1;
            continue;
        }

        struct ifreq ifr;
        strcpy(ifr.ifr_name, deviceList[i].c_str());
        if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0) {
            LogError(SERIAL_TAG, "get CAN interface index failed for %s", deviceList[i].c_str());
            close(sockfd);
            sendResult[i] = false;
            recvResult[i] = false;
            sockfds[i] = -1;
            continue;
        }

        struct sockaddr_can addr;
        memset(&addr, 0, sizeof(addr));
        addr.can_family = AF_CAN;
        addr.can_ifindex = ifr.ifr_ifindex;

        if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            LogError(SERIAL_TAG, "bind CAN socket failed for %s", deviceList[i].c_str());
            close(sockfd);
            sendResult[i] = false;
            recvResult[i] = false;
            sockfds[i] = -1;
            continue;
        }

        // 设置CAN套接字为非阻塞模式，避免读取时卡住
        int flags = fcntl(sockfd, F_GETFL, 0);
        if (flags == -1) {
            LogError(SERIAL_TAG, "get socket flags failed for %s", deviceList[i].c_str());
            close(sockfd);
            sendResult[i] = false;
            recvResult[i] = false;
            sockfds[i] = -1;
            continue;
        }
        if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
            LogError(SERIAL_TAG, "set socket non-blocking failed for %s", deviceList[i].c_str());
            close(sockfd);
            sendResult[i] = false;
            recvResult[i] = false;
            sockfds[i] = -1;
            continue;
        }

        sockfds[i] = sockfd;
        LogDebug(SERIAL_TAG, "open CAN device %s success, sockfd = %d", deviceList[i].c_str(), sockfd);
    }

    // CAN测试数据
    struct can_frame testFrame;
    testFrame.can_id = 0x123;        // 测试用的CAN ID
    testFrame.can_dlc = 8;           // 数据长度
    strcpy((char*)testFrame.data, "TESTDATA"); // 测试数据

    struct can_frame recvFrame;
    
    // 测试逻辑：每个设备依次发送数据，其他设备接收
    for (int i = 0; i < deviceList.size(); i++) {       // 每个设备依次发送数据
        if (sockfds[i] == -1) continue;                 // 跳过打开失败的设备
        
        for (int j = 0; j < deviceList.size(); j++) {   // 其他设备接收数据
            if (j == i || sockfds[j] == -1) continue;   // 跳过自己和打开失败的设备

            log_thread_safe(LOG_LEVEL_INFO, SERIAL_TAG, "CAN Device %s sending frame to Device %s", deviceList[i].c_str(), deviceList[j].c_str());

            for (int k = 0; k < testCount; k++) {
                ssize_t bytesWritten = -1;
                ssize_t bytesRead = -1;
                
                // 发送CAN帧
                bytesWritten = write(sockfds[i], &testFrame, sizeof(testFrame));
                if (bytesWritten != sizeof(testFrame)) {
                    LogError(SERIAL_TAG, "write to %s failed. ret = %d, expected = %zu",
                             deviceList[i].c_str(), bytesWritten, sizeof(testFrame));
                    sendResult[i] = false;              // 标记发送失败
                    continue;                           // 尝试下一次发送
                }
                
                usleep(10000);  // 等待10ms，确保数据传输

                // 接收CAN帧 - 使用非阻塞读取和超时机制
                memset(&recvFrame, 0, sizeof(recvFrame));
                bytesRead = -1;
                
                // 使用select实现超时机制
                fd_set readfds;
                struct timeval timeout;
                
                FD_ZERO(&readfds);
                FD_SET(sockfds[j], &readfds);
                timeout.tv_sec = 1;  // 1秒超时
                timeout.tv_usec = 0;
                
                int selectResult = select(sockfds[j] + 1, &readfds, NULL, NULL, &timeout);
                
                if (selectResult == -1) {
                    LogError(SERIAL_TAG, "select failed for %s: %s", deviceList[j].c_str(), strerror(errno));
                    recvResult[j] = false;
                    break;
                } else if (selectResult == 0) {
                    // 超时，没有数据可读
                    LogError(SERIAL_TAG, "read from %s timeout, no data received", deviceList[j].c_str());
                    recvResult[j] = false;
                    break;
                } else {
                    // 有数据可读
                    bytesRead = read(sockfds[j], &recvFrame, sizeof(recvFrame));
                    
                    if (bytesRead < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // 非阻塞模式下没有数据
                            LogError(SERIAL_TAG, "no data available from %s", deviceList[j].c_str());
                        } else {
                            LogError(SERIAL_TAG, "read from %s failed: %s", deviceList[j].c_str(), strerror(errno));
                        }
                        recvResult[j] = false;
                        break;
                    } else if (bytesRead == sizeof(recvFrame)) {
                        LogDebug(SERIAL_TAG, "read from %s: id=0x%X, data=%.8s",
                                 deviceList[j].c_str(), recvFrame.can_id, recvFrame.data);
                        
                        // 验证接收到的CAN帧
                        if (recvFrame.can_id == testFrame.can_id &&
                            recvFrame.can_dlc == testFrame.can_dlc &&
                            memcmp(recvFrame.data, testFrame.data, testFrame.can_dlc) == 0) {
                            continue;                   // 成功接收到正确数据
                        } else {
                            recvResult[j] = false;      // 标记接收失败
                            LogError(SERIAL_TAG, "invalid CAN frame from %s to %s",
                                     deviceList[i].c_str(), deviceList[j].c_str());
                            break;                      // 下一个设备
                        }
                    } else {
                        recvResult[j] = false;          // 标记接收失败
                        LogError(SERIAL_TAG, "incomplete CAN frame received from %s, bytes=%d",
                                 deviceList[j].c_str(), bytesRead);
                        break;                          // 下一个设备
                    }
                }
            }
        }
    }

    // 关闭所有打开的CAN套接字
    for (int i = 0; i < sockfds.size(); i++) {
        if (sockfds[i] != -1) {
            close(sockfds[i]);
            sockfds[i] = -1;
        }
    }
    
    return true;
}

bool Serial::serialCanTestRetry(std::vector<std::string>& deviceList, std::vector<bool>& sendResult, std::vector<bool>& recvResult, 
                                int reCount, int reCountTimeUs, int testCount) {
    if (deviceList.size() < 2) {                     // CAN 测试，至少需要两个设备
        LogError(SERIAL_TAG, "deviceList size < 2");
        return false;
    }
    

    
    // 测试逻辑：每个设备依次发送数据，其他设备接收
    for (int attempt = 0; attempt <= reCount; attempt++) {   // 重试机制
        std::vector<int> sockfds(deviceList.size(), -1); // 存储每个设备的套接字描述符
        sendResult.resize(deviceList.size(), true);      // 初始化发送结果, 默认成功
        recvResult.resize(deviceList.size(), true);      // 初始化接收结果, 默认成功

        for (int i = 0; i < deviceList.size(); i++) {   // 重置结果
            sendResult[i] = true;
            recvResult[i] = true;
        }

        std::cout << "测试结果置位：发送全部成功，接收全部成功" << std::endl;
        for (int i = 0; i < deviceList.size(); i++) {
            std::cout << "Device " << deviceList[i] << ": Send Result = " 
                      << (sendResult[i] ? "Success" : "Failed") 
                      << ", Recv Result = " 
                      << (recvResult[i] ? "Success" : "Failed") 
                      << std::endl;
        }

        // 打开所有CAN设备
        for (int i = 0; i < deviceList.size(); i++) {
            //ip link set down can0 
            //ip link set can0 type can bitrate 250000
            //ip link set up can0
            int ret = system(("ip link set down " + deviceList[i]).c_str());    // 无法使用系统命令设置时，收发结果置位false, system命令不靠谱，后期需要改为ioctl方式
            if (ret != 0) {
                sendResult[i] = false;
                recvResult[i] = false;
                LogError(SERIAL_TAG, "set down %s failed", deviceList[i].c_str());
            }
            ret = system(("ip link set " + deviceList[i] + " type can bitrate 250000").c_str());
            if (ret != 0) {
                sendResult[i] = false;
                recvResult[i] = false;
                LogError(SERIAL_TAG, "set bitrate for %s failed", deviceList[i].c_str());
            }

            system(("ip link set up " + deviceList[i]).c_str());
            if (ret != 0) {
                sendResult[i] = false;
                recvResult[i] = false;
                LogError(SERIAL_TAG, "set up %s failed", deviceList[i].c_str());
            }

            int sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
            if (sockfd < 0) {
                LogError(SERIAL_TAG, "create CAN socket failed for %s", deviceList[i].c_str());
                sendResult[i] = false;
                recvResult[i] = false;
                sockfds[i] = -1;
                continue;
            }

            struct ifreq ifr;
            strcpy(ifr.ifr_name, deviceList[i].c_str());
            if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0) {
                LogError(SERIAL_TAG, "get CAN interface index failed for %s", deviceList[i].c_str());
                close(sockfd);
                sendResult[i] = false;
                recvResult[i] = false;
                sockfds[i] = -1;
                continue;
            }

            struct sockaddr_can addr;
            memset(&addr, 0, sizeof(addr));
            addr.can_family = AF_CAN;
            addr.can_ifindex = ifr.ifr_ifindex;

            if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
                LogError(SERIAL_TAG, "bind CAN socket failed for %s", deviceList[i].c_str());
                close(sockfd);
                sendResult[i] = false;
                recvResult[i] = false;
                sockfds[i] = -1;
                continue;
            }

            // 设置CAN套接字为非阻塞模式，避免读取时卡住
            int flags = fcntl(sockfd, F_GETFL, 0);
            if (flags == -1) {
                LogError(SERIAL_TAG, "get socket flags failed for %s", deviceList[i].c_str());
                close(sockfd);
                sendResult[i] = false;
                recvResult[i] = false;
                sockfds[i] = -1;
                continue;
            }
            if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
                LogError(SERIAL_TAG, "set socket non-blocking failed for %s", deviceList[i].c_str());
                close(sockfd);
                sendResult[i] = false;
                recvResult[i] = false;
                sockfds[i] = -1;
                continue;
            }

            sockfds[i] = sockfd;
            LogDebug(SERIAL_TAG, "open CAN device %s success, sockfd = %d", deviceList[i].c_str(), sockfd);
        }

        // CAN测试数据
        struct can_frame testFrame;
        testFrame.can_id = 0x123;        // 测试用的CAN ID
        testFrame.can_dlc = 8;           // 数据长度
        strcpy((char*)testFrame.data, "TESTDATA"); // 测试数据

        struct can_frame recvFrame;

        for (int i = 0; i < deviceList.size(); i++) {       // 每个设备依次发送数据
            if (sockfds[i] == -1) continue;                 // 跳过打开失败的设备
            
            for (int j = 0; j < deviceList.size(); j++) {   // 其他设备接收数据
                if (j == i || sockfds[j] == -1) continue;   // 跳过自己和打开失败的设备
                
                log_thread_safe(LOG_LEVEL_INFO, SERIAL_TAG, "CAN Device %s sending frame to Device %s", deviceList[i].c_str(), deviceList[j].c_str());

                for (int k = 0; k < testCount; k++) {
                    ssize_t bytesWritten = -1;
                    ssize_t bytesRead = -1;
                    
                    // 发送CAN帧
                    bytesWritten = write(sockfds[i], &testFrame, sizeof(testFrame));
                    if (bytesWritten != sizeof(testFrame)) {
                        LogError(SERIAL_TAG, "write to %s failed. ret = %d, expected = %zu",
                                deviceList[i].c_str(), bytesWritten, sizeof(testFrame));
                        sendResult[i] = false;              // 标记发送失败
                        continue;                           // 尝试下一次发送
                    }
                    
                    usleep(10000);  // 等待10ms，确保数据传输

                    // 接收CAN帧 - 使用非阻塞读取和超时机制
                    memset(&recvFrame, 0, sizeof(recvFrame));
                    bytesRead = -1;
                    
                    // 使用select实现超时机制
                    fd_set readfds;
                    struct timeval timeout;
                    
                    FD_ZERO(&readfds);
                    FD_SET(sockfds[j], &readfds);
                    timeout.tv_sec = 1;  // 1秒超时
                    timeout.tv_usec = 0;
                    
                    int selectResult = select(sockfds[j] + 1, &readfds, NULL, NULL, &timeout);
                    
                    if (selectResult == -1) {
                        LogError(SERIAL_TAG, "select failed for %s: %s", deviceList[j].c_str(), strerror(errno));
                        recvResult[j] = false;
                        break;
                    } else if (selectResult == 0) {
                        // 超时，没有数据可读
                        LogError(SERIAL_TAG, "read from %s timeout, no data received", deviceList[j].c_str());
                        recvResult[j] = false;
                        break;
                    } else {
                        // 有数据可读
                        bytesRead = read(sockfds[j], &recvFrame, sizeof(recvFrame));
                        
                        if (bytesRead < 0) {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                // 非阻塞模式下没有数据
                                LogError(SERIAL_TAG, "no data available from %s", deviceList[j].c_str());
                            } else {
                                LogError(SERIAL_TAG, "read from %s failed: %s", deviceList[j].c_str(), strerror(errno));
                            }
                            recvResult[j] = false;
                            break;
                        } else if (bytesRead == sizeof(recvFrame)) {
                            LogDebug(SERIAL_TAG, "read from %s: id=0x%X, data=%.8s",
                                    deviceList[j].c_str(), recvFrame.can_id, recvFrame.data);
                            
                            // 验证接收到的CAN帧
                            if (recvFrame.can_id == testFrame.can_id &&
                                recvFrame.can_dlc == testFrame.can_dlc &&
                                memcmp(recvFrame.data, testFrame.data, testFrame.can_dlc) == 0) {
                                continue;                   // 成功接收到正确数据
                            } else {
                                recvResult[j] = false;      // 标记接收失败
                                LogError(SERIAL_TAG, "invalid CAN frame from %s to %s",
                                        deviceList[i].c_str(), deviceList[j].c_str());
                                break;                      // 下一个设备
                            }
                        } else {
                            recvResult[j] = false;          // 标记接收失败
                            LogError(SERIAL_TAG, "incomplete CAN frame received from %s, bytes=%d",
                                    deviceList[j].c_str(), bytesRead);
                            break;                          // 下一个设备
                        }
                    }
                }
            }
        }

        // 关闭所有打开的CAN套接字
        for (int i = 0; i < sockfds.size(); i++) {
            if (sockfds[i] != -1) {
                close(sockfds[i]);
                sockfds[i] = -1;
            }
        }

        for (int i = 0; i < deviceList.size(); i++) {   // 打印本次测试结果
            std::cout << "Device " << deviceList[i] << ": Send Result = " 
                      << (sendResult[i] ? "Success" : "Failed") 
                      << ", Recv Result = " 
                      << (recvResult[i] ? "Success" : "Failed") 
                      << std::endl;
        }

        for (int i = 0; i < deviceList.size(); i++) {
            if (sendResult[i] == false || recvResult[i] == false) {
                LogDebug(SERIAL_TAG, "serial CAN test failed on attempt %d, retrying...", attempt + 1);
                usleep(reCountTimeUs);   // 等待一段时间后重试
                break;
            }
            if (i == deviceList.size() - 1) {
                LogDebug(SERIAL_TAG, "serial CAN test success on attempt %d", attempt + 1);
                return true;  // 测试成功，退出重试循环
            }
        }
    }

    return true;
}

/*
/# ip link set can2 down
/# ip link set can2 type can bitrate 250000
/# ip link set can2 up
*/
int Serial::open_can_device(const char* can_interface_name)
{
    if (!can_interface_name) {
        log_thread_safe(LOG_LEVEL_ERROR, SERIAL_TAG, "CAN interface name is null");
        return -1;
    }

    char cmd[128];

    // Bring down interface
    snprintf(cmd, sizeof(cmd), "ip link set %s down", can_interface_name);
    system(cmd);

    // Set bitrate 250kbps
    snprintf(cmd, sizeof(cmd), "ip link set %s type can bitrate 250000", can_interface_name);
    if (system(cmd) != 0) {
        log_thread_safe(LOG_LEVEL_ERROR, SERIAL_TAG, "set bitrate failed");
        return -1;
    }

    // Bring up interface
    snprintf(cmd, sizeof(cmd), "ip link set %s up", can_interface_name);
    if (system(cmd) != 0) {
        log_thread_safe(LOG_LEVEL_ERROR, SERIAL_TAG, "bring up failed");
        return -1;
    }

    // Create socket
    int fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (fd < 0) {
        log_thread_safe(LOG_LEVEL_ERROR, SERIAL_TAG, "socket failed: %s", strerror(errno));
        return -1;
    }

    // Get interface index
    struct ifreq ifr {};
    strncpy(ifr.ifr_name, can_interface_name, IFNAMSIZ - 1);
    if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
        log_thread_safe(LOG_LEVEL_ERROR, SERIAL_TAG, "SIOCGIFINDEX failed: %s", strerror(errno));
        close(fd);
        return -1;
    }

    // Bind socket
    struct sockaddr_can addr {};
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        log_thread_safe(LOG_LEVEL_ERROR, SERIAL_TAG, "bind failed: %s", strerror(errno));
        close(fd);
        return -1;
    }

    // Set non-blocking
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    return fd;
}


bool Serial::can_send(int fd, uint32_t can_id, const uint8_t* data)
{
    if (fd < 0 || !data) {
        return false;
    }

    struct can_frame frame {};
    frame.can_id  = can_id;
    frame.can_dlc = 6;
    memcpy(frame.data, data, 6);

    int n = write(fd, &frame, sizeof(frame));
    if (n != sizeof(frame)) {
        log_thread_safe(LOG_LEVEL_ERROR, SERIAL_TAG, "CAN write failed: %s", strerror(errno));
        return false;
    }

    return true;
}


int Serial::can_recv(int fd, unsigned char* buf, int& len, int timeout_ms)
{
    len = 0;
    if (fd < 0 || !buf) {
        log_thread_safe(LOG_LEVEL_ERROR, SERIAL_TAG, "Invalid fd or buf");
        return -1;
    }

    // poll 等待
    struct pollfd pfd {};
    pfd.fd     = fd;
    pfd.events = POLLIN;

    int ret = poll(&pfd, 1, timeout_ms);
    if (ret == 0) {
        // 超时
        return 0;
    } else if (ret < 0) {
        log_thread_safe(LOG_LEVEL_ERROR, SERIAL_TAG, "poll error: %s", strerror(errno));
        return -1;
    }

    if (!(pfd.revents & POLLIN)) {
        return 0;
    }

    // 读取 CAN 帧
    struct can_frame frame {};
    int n = read(fd, &frame, sizeof(frame));
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        log_thread_safe(LOG_LEVEL_ERROR, SERIAL_TAG, "CAN read error: %s", strerror(errno));
        return -1;
    }

    if (n != sizeof(frame)) {
        log_thread_safe(LOG_LEVEL_WARN, SERIAL_TAG, "Incomplete CAN frame received");
        return -1;
    }

    len = frame.can_dlc;
    memcpy(buf, frame.data, len);

    return len;
}



bool Serial::can_test(int fd, int test_count)
{
    const uint8_t tx_data[32] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};

    for (int i = 0; i < test_count; ++i) {
        if (!can_send(fd, 0x001, tx_data)) {
            log_thread_safe(LOG_LEVEL_WARN, SERIAL_TAG, "CAN send failed");
            continue;
        }
        log_thread_safe(LOG_LEVEL_INFO, SERIAL_TAG, "CAN test sent fd = %d: %02X %02X %02X %02X %02X %02X %02X %02X",
            fd, tx_data[0], tx_data[1], tx_data[2], tx_data[3],
            tx_data[4], tx_data[5], tx_data[6], tx_data[7]);
        uint8_t buf[32];
        int len = 0;

        int ret = can_recv(fd, buf, len, 1000);  // 200ms 超时
        if (ret <= 0) {
            log_thread_safe(LOG_LEVEL_WARN, SERIAL_TAG, "CAN reply timeout or error : fd=%d ret=%d", fd, ret);
            continue;
        }

        log_thread_safe(LOG_LEVEL_INFO, SERIAL_TAG, "CAN test received fd = %d: data len=%d",
            fd, len);
        //     log_thread_safe(LOG_LEVEL_WARN, SERIAL_TAG, "Invalid DLC=%d", len);
        //     continue;
        // }

        // 对方第一个字节改成 0x02

        if (len < 6) {
            log_thread_safe(LOG_LEVEL_WARN, SERIAL_TAG, "CAN reply too short: len=%d", len);
            continue;
        }

        log_thread_safe(LOG_LEVEL_INFO, SERIAL_TAG, "CAN test received fd = %d: %02X %02X %02X %02X %02X %02X %02X %02X",
            fd, buf[0], buf[1], buf[2], buf[3],
            buf[4], buf[5], buf[6], buf[7]);

        if (buf[0] == 0x02 && memcmp(buf + 1, tx_data + 1, 5) == 0) {
            log_thread_safe(LOG_LEVEL_INFO, SERIAL_TAG, "CAN test OK");
            return true;
        }

        // log_thread_safe(LOG_LEVEL_WARN, SERIAL_TAG,
        //     "CAN reply mismatch: %02X %02X %02X %02X %02X %02X %02X %02X",
        //     buf[0], buf[1], buf[2], buf[3],
        //     buf[4], buf[5]);
    }

    return false;
}






int Serial::open_485_device(const char* device) {
    return openSerial(device, 115200);
}

bool Serial::serial_485_send(int fd, const unsigned char* buf, size_t len) {
    if (fd < 0 || buf == nullptr || len == 0) {
        log_thread_safe(LOG_LEVEL_INFO, SERIAL_TAG, "Invalid fd, buf or len");
        return false;
    }

    ssize_t bytesWritten = write(fd, buf, len);
    if (bytesWritten < 0 || static_cast<size_t>(bytesWritten) != len) {
        log_thread_safe(LOG_LEVEL_INFO, SERIAL_TAG, "Serial 485 write failed: %s", strerror(errno));
        return false;
    }

    return true;
}

bool Serial::serial_485_recv(int fd, unsigned char* buf, size_t len) {
    if (fd < 0 || buf == nullptr || len == 0) {
        log_thread_safe(LOG_LEVEL_INFO, SERIAL_TAG, "Invalid fd, buf or len");
        return false;
    }

    ssize_t bytesRead = read(fd, buf, len);
    if (bytesRead < 0) {
        log_thread_safe(LOG_LEVEL_INFO, SERIAL_TAG, "Serial 485 read failed: %s", strerror(errno));
        return false;
    } else if (bytesRead == 0) {
        log_thread_safe(LOG_LEVEL_INFO, SERIAL_TAG, "Serial 485 read no data");
        return false;
    }

    buf[bytesRead] = '\0'; // Null-terminate the received data
    log_thread_safe(LOG_LEVEL_INFO, SERIAL_TAG, "Serial 485 received data: %s", buf);

    return true;
}

bool Serial::serial_485_test(int fd, int test_count) {
    if (fd < 0) {
        log_thread_safe(LOG_LEVEL_INFO, SERIAL_TAG, "serial 485 test: Invalid fd");
        return false;
    }

    for (int i = 0; i < test_count; i++) {
        bool ret = serial_485_send(fd, (unsigned char *)"TESTDATA", 8);
        if (!ret) {
            continue;
        }

        usleep(100000);

        unsigned char buf[32];
        ret = serial_485_recv(fd, buf, sizeof(buf) - 1);
        if (!ret) {
            continue;
        }

        if (memcmp(buf, "TESTDATA", 8) == 0) {
            return true;
        } else {
            continue;
        }
    }
}