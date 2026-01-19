#ifndef PROTOCOL_PARSER_H
#define PROTOCOL_PARSER_H

#include "protocol/BufferManager.h"
#include "protocol/Crc16.h"
#include "common/Constants.h"
#include "common/Types.h"
#include "util/JsonHelper.h"
#include <vector>
#include <string>
#include <cstdint>
#include <mutex>

class Uart; // 前向声明

class ProtocolParser {
public:
    ProtocolParser(Uart* uart);
    ~ProtocolParser() = default;

    // 写入数据到缓冲区
    void writeBuffer(const uint8_t* data, uint16_t len);

    // 解析协议帧（返回：0-未完成，1-解析成功，-1-错误）
    int parseFrame();

    // 打包数据（返回：是否成功）
    bool packData(std::vector<uint8_t>& pack, const std::string& jsonData, uint16_t cmdIndex);

    // 获取解析后的任务
    Task getTask() const;

    // 发送响应
    bool sendResponse(const Json::Value& root, CmdType cmdType);
    bool sendResponse(const Json::Value& root, int cmdIndex);
    bool sendResponse(const Json::Value& root);
    
    //回复索引
    uint16_t reportCmdIndex_;

    // 重置解析状态
    void reset();

private:
    // 状态机枚举
    enum ParseState {
        STATE_IDLE,              // 初始状态
        STATE_FIND_HEADER,       // 查找帧头
        STATE_PARSE_HEADER,      // 解析帧头字段
        STATE_CHECK_FRAME_COMPLETE, // 检查帧是否完整
        STATE_VERIFY_CRC,        // 校验CRC
        STATE_CHECK_DUPLICATE,   // 检查重复数据
        STATE_PARSE_JSON,        // 解析JSON数据
        STATE_BUILD_TASK,        // 构造任务
        STATE_CONSUME_DATA,      // 消费数据
        STATE_ERROR              // 错误状态
    };

    mutable std::mutex mutex_;      // 互斥锁
    Uart* uart_;                    // 串口引用
    BufferManager buffer_;          // 缓冲区管理
    Task currentTask_;              // 当前任务
    Json::Value jsonRoot_;          // 解析后的JSON数据
    uint16_t lastCmdIndex_;         // 上一次命令索引（去重）
    Crc16::Calculator crcCalculator_;  // CRC计算器
    ParseState state_;              // 当前解析状态
    uint16_t frameStart_;           // 帧起始位置
    uint16_t cmdIndex_;             // 当前命令索引
    uint16_t contentLen_;           // 内容长度
    uint16_t totalFrameLen_;        // 总帧长度

    // 解析JSON数据
    bool parseJson(const std::string& jsonStr);
    
    // 字符串转测试类型
    TestItem stringToTestItem(const std::string& str);

    const char *PROTOCOL_TAG = "ProtocolParser";
};

#endif // PROTOCOL_PARSER_H
