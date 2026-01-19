#ifndef BASE_TEST_H
#define BASE_TEST_H

#include "hardware/TestInterface.h"

class BaseTest : public TestInterface {
public:
    BaseTest() = default;
    ~BaseTest() override = default;
    
    TestResult execute(const Json::Value& params) override;
    TestItem getType() const override { return BASE; }
    std::string getName() const override { return "base"; }

private:
    // 获取硬件ID
    bool getHardwareId(std::string& hwid);
};

#endif // BASE_TEST_H
