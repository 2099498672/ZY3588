#ifndef TEST_INTERFACE_H
#define TEST_INTERFACE_H

#include "common/Types.h"
#include "common/Constants.h"
#include <memory>
#include <string>

// 硬件测试抽象接口
class TestInterface {
public:
    virtual ~TestInterface() = default;
    
    // 执行测试（输入测试参数JSON，输出测试结果JSON）
    virtual TestResult execute(const Json::Value& params) = 0;
    
    // 获取测试类型
    virtual TestItem getType() const = 0;
    
    // 获取测试名称
    virtual std::string getName() const = 0;
};

// 测试工厂类（创建测试实例）
class TestFactory {
public:
    // 根据测试类型创建测试实例
    static std::unique_ptr<TestInterface> create(TestItem type);
};

#endif // TEST_INTERFACE_H
