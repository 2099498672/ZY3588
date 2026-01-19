#include "Uart.h"
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

Uart::Uart(const std::string& device) : device_(device), fd_(-1) {
    log_thread_safe(LOG_LEVEL_INFO, UART_TAG, "串口设备: %s", device_.c_str());
}

Uart::~Uart() {
    close();
}

bool Uart::open() {
    if (fd_ != -1) {
        log_thread_safe(LOG_LEVEL_WARN, UART_TAG, "串口已打开");
        return false;
    }
    
    fd_ = ::open(device_.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd_ == -1) {
        log_thread_safe(LOG_LEVEL_ERROR, UART_TAG, "无法打开串口: %s, 错误: %s", device_.c_str(), strerror(errno));
        return false;
    }

    // 配置为非阻塞模式
    // if (fcntl(fd_, F_SETFL, O_NONBLOCK) == -1) {
    //     std::cerr << "无法配置串口为非阻塞模式: " << strerror(errno) << std::endl;
    //     ::close(fd_);
    //     fd_ = -1;
    //     return false;
    // }
    
    // 检查是否为终端设备
    // if (!isatty(fd_)) {
    //     std::cerr << device_ << " 不是终端设备" << std::endl;
    //     ::close(fd_);
    //     fd_ = -1;
    //     return false;
    // }
    return true;
}

void Uart::close() {
    if (fd_ != -1) {
        ::close(fd_);
        fd_ = -1;
    }
}

bool Uart::configure(int baudrate, int databits, char parity, int stopbits) {
    if (fd_ == -1) {
        log_thread_safe(LOG_LEVEL_ERROR, UART_TAG, "串口未打开");
        return false;
    }
    
    termios options;
    if (tcgetattr(fd_, &options) != 0) {
        log_thread_safe(LOG_LEVEL_ERROR, UART_TAG, "无法获取串口属性: %s", strerror(errno));
        return false;
    }

    speed_t speed;
    switch (baudrate) {
        case 9600: speed = B9600; break;
        case 19200: speed = B19200; break;
        case 38400: speed = B38400; break;
        case 57600: speed = B57600; break;
        case 115200: speed = B115200; break;
        default:
            log_thread_safe(LOG_LEVEL_ERROR, UART_TAG, "不支持的波特率: %d", baudrate);
            return false;
    }
    
    if (cfsetispeed(&options, speed) != 0 && cfsetospeed(&options, speed) != 0) {
        log_thread_safe(LOG_LEVEL_ERROR, UART_TAG, "无法设置波特率: %s", strerror(errno));
        return false;
    }
    
    options.c_cflag |= (CLOCAL | CREAD); // 启用接收器和本地模式
    
    options.c_cflag &= ~CSIZE;
    switch (databits) {
        case 5: options.c_cflag |= CS5; break;
        case 6: options.c_cflag |= CS6; break;
        case 7: options.c_cflag |= CS7; break;
        case 8: options.c_cflag |= CS8; break;
        default:
            log_thread_safe(LOG_LEVEL_ERROR, UART_TAG, "不支持的数据位: %d", databits);
            return false;
    }
    
    switch (parity) {
        case 'N': case 'n':
            options.c_cflag &= ~PARENB; // 无校验
            options.c_iflag &= ~INPCK;
            break;
        case 'O': case 'o':
            options.c_cflag |= (PARODD | PARENB); // 奇校验
            options.c_iflag |= INPCK;
            break;
        case 'E': case 'e':
            options.c_cflag |= PARENB;
            options.c_cflag &= ~PARODD; // 偶校验
            options.c_iflag |= INPCK;
            break;
        default:
            log_thread_safe(LOG_LEVEL_ERROR, UART_TAG, "不支持的校验位: %c", parity);
            return false;
    }
    
    if (stopbits == 1) {
        options.c_cflag &= ~CSTOPB;
    } else if (stopbits == 2) {
        options.c_cflag |= CSTOPB;
    } else {
        log_thread_safe(LOG_LEVEL_ERROR, UART_TAG, "不支持的停止位: %d", stopbits);
        return false;
    }
    

    options.c_cflag |= (CLOCAL | CREAD);  // 启用接收器和本地模式
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // 原始模式
    options.c_oflag &= ~OPOST; // 原始输出
    options.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL | INLCR | IGNCR); // 禁用软件流控制和字符转换
    
    options.c_cc[VTIME] = 1;   // 10 x 100ms = 1s
    options.c_cc[VMIN] = 0;     // 最小字符数为0，非阻塞读取
    
    if (tcsetattr(fd_, TCSANOW, &options) != 0) {
        log_thread_safe(LOG_LEVEL_ERROR, UART_TAG, "无法应用串口配置: %s", strerror(errno));
        return false;
    }
    
    return true;
}

ssize_t Uart::sendData(const uint8_t* data, size_t len) {
    if (data == nullptr || len == 0) {
        log_thread_safe(LOG_LEVEL_WARN, UART_TAG, "发送数据为空或长度为0");
        return 0;
    }

    if (fd_ == -1) {
        log_thread_safe(LOG_LEVEL_ERROR, UART_TAG, "串口未打开");
        return -1;
    }
    
    ssize_t sent = ::write(fd_, data, len);

    if (sent == -1) {
        log_thread_safe(LOG_LEVEL_ERROR, UART_TAG, "发送数据失败: %s", strerror(errno));
    }
    
    return sent;
}

ssize_t Uart::receiveData(uint8_t* buffer, size_t maxLen) {
    if (buffer == nullptr || maxLen == 0) {
        return 0;
    }

    if (fd_ == -1) {
        log_thread_safe(LOG_LEVEL_ERROR, UART_TAG, "串口未打开");
        return -1;
    }
    
    ssize_t received = ::read(fd_, buffer, maxLen);
    if (received == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
        log_thread_safe(LOG_LEVEL_ERROR, UART_TAG, "接收数据失败: %s", strerror(errno));
        return -1;
    }
    
    return received;
}

bool Uart::isOpen() const {
    return fd_ != -1;
}
