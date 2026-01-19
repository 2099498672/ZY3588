#ifndef UART_H
#define UART_H

#include <string>
#include <cstdint>
#include <mutex>

#include "util/Log.h"

class Uart {
public:
    explicit Uart(const std::string& device);
    ~Uart();
    Uart(const Uart&) = delete;
    Uart& operator=(const Uart&) = delete;
    bool open();
    void close();
    bool configure(int baudrate, int databits, char parity, int stopbits);
    ssize_t sendData(const uint8_t* data, size_t len);
    ssize_t receiveData(uint8_t* buffer, size_t maxLen);
    bool isOpen() const;

private:
    const char * UART_TAG = "UART";
    std::string device_;  
    int fd_;             
};

#endif // UART_H
