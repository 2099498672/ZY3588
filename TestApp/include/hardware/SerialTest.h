#ifndef SERIAL_TEST_H
#define SERIAL_TEST_H

#include "hardware/TestInterface.h"
#include "Uart.h"
#include <thread>
#include <atomic>

class SerialTest : public TestInterface {
public:
    SerialTest() = default;
    ~SerialTest() override = default;
    
    TestResult execute(const Json::Value& params) override;
    TestItem getType() const override { return SERIAL; }
    std::string getName() const override { return "serial"; }

private:
    // 串口发送线程函数
    void sendThreadFunc(int fd, const std::string& data, int count, 
                       std::atomic<int>& sent, std::atomic<int>& errors);
    
    // 串口接收线程函数
    void receiveThreadFunc(int fd, const std::string& expectedData, 
                          std::atomic<int>& received, std::atomic<int>& errors);
};

#endif // SERIAL_TEST_H
