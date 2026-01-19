#ifndef TYPES_H
#define TYPES_H

#include "json/json.h"
#include "common/Constants.h"

// 任务结构体
struct Task {
    SubCommand subCommand;          // 子命令
    Json::Value data;               // 测试参数
    uint16_t cmdIndex;              // 命令索引
};

// 测试结果结构体
struct TestResult {
    bool success;                   // 是否成功
    ErrorCode errorCode;            // 错误码
    std::string errorMsg;           // 错误信息
    Json::Value data;               // 测试数据
    SubCommand responseCommand;     // 响应命令
    uint16_t cmdIndex;              // 对应的命令索引
};

#endif // TYPES_H

