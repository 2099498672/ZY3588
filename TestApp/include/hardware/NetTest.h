#ifndef NET_TEST_H
#define NET_TEST_H

#include "hardware/TestInterface.h"

class NetTest : public TestInterface {
public:
    NetTest() = default;
    ~NetTest() override = default;
    
    TestResult execute(const Json::Value& params) override;
    TestItem getType() const override { return NET; }
    std::string getName() const override { return "net"; }

private:
    // Ping测试
    bool pingTest(const std::string& host, int timeoutMs);
};

#endif // NET_TEST_H
