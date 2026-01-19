#ifndef TASK_HANDLER_H
#define TASK_HANDLER_H
#define TEST_APP_VERSION "CMBOARD_TESTAPP_CM3588_V20251011_221400"

#include <thread>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "common/Types.h"
#include "task/TaskQueue.h"
#include "protocol/ProtocolParser.h"
#include "hardware/TestInterface.h"
#include "hardware/RkGenericBoard.h"
#include "util/Log.h"

/**
 * 任务处理器，负责执行测试任务并处理结果
 */
class TaskHandler {
public:
    /**
     * 构造函数
     * @param protocol 协议解析器引用，用于发送响应
     */
    explicit TaskHandler(ProtocolParser& protocol);

    // 处理任务
    void processTask(const Task& task, std::shared_ptr<RkGenericBoard> Board);

    // 回复握手命令
    void handleHandshake(const Task& task);    // 0x01

    // 回复开始测试命令
    void handleBeginTest(const Task& task);    // 0x02

    // 回复结束测试命令
    void handleOverTest(const Task& task);     // 0x03

    // 回复单个待测测试命令
    void handleSignalToBeMeasured(const Task& task);  // 0x05

    // 设置系统时间命令
    void handleSetSysTime(const Task& task);   // 0x0A

    // 回复单个执行命令
    void handleSignalExec(const Task& task);     // 0x0D

    // 回复单个测试组合命令
    void handleSignalToBeMeasuredCombine(const Task& task);   // 0x0F


    // 字符串转换为测试项目枚举
    TestItem stringToTestItem(const std::string& str);
    

    void storage_test(const Task& task, std::shared_ptr<RkGenericBoard> Board);
    void switchs_test(const Task& task, std::shared_ptr<RkGenericBoard> Board);
    void serial_test(const Task& task, std::shared_ptr<RkGenericBoard> Board);
    void net_test(const Task& task, std::shared_ptr<RkGenericBoard> Board);
    void rtc_test(const Task& task, std::shared_ptr<RkGenericBoard> Board);
    void wifi_test(const Task& task, std::shared_ptr<RkGenericBoard> Board);
    void bluetooth_test(const Task& task, std::shared_ptr<RkGenericBoard> Board);
    void gpio_test(const Task& task, std::shared_ptr<RkGenericBoard> Board);
    void can_test(const Task& task, std::shared_ptr<RkGenericBoard> Board);
    void fan_test(const Task& task, std::shared_ptr<RkGenericBoard> Board);
    void manual_test(const Task& task, std::shared_ptr<RkGenericBoard> Board);
    void camera_test(const Task& task, std::shared_ptr<RkGenericBoard> Board);
    void typec_test(const Task& task, std::shared_ptr<RkGenericBoard> Board);
    void ln_test(const Task& task, std::shared_ptr<RkGenericBoard> Board);
    void baseinfo_test(const Task& task, std::shared_ptr<RkGenericBoard> Board);
    void microphone_test(const Task& task, std::shared_ptr<RkGenericBoard> Board);
    void common_test(const Task& task, std::shared_ptr<RkGenericBoard> Board);

    void stop_all_tasks();
    
private:
    ProtocolParser& protocol_;  // 协议解析器引用
    
    // 线程管理
    std::atomic<bool> keyThreadStopFlag_;      // 按键线程停止标志
    std::shared_ptr<std::thread> keyThread_;   // 按键检测线程
    std::mutex keyThreadMutex_;                // 线程互斥锁

    // 执行测试并发送结果
    void executeTestAndRespond(const Task& task, std::shared_ptr<RkGenericBoard> Board);

    // 构建响应
    Json::Value buildResponse(const Task& task, const TestResult& result, const std::string& testName);
    
    const char * TaskHandlerTag = "TaskHandler";
};

#endif // TASK_HANDLER_H
