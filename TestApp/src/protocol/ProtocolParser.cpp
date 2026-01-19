#include "protocol/ProtocolParser.h"
#include "Uart.h"
#include <iostream>
#include <chrono>
#include <random>

ProtocolParser::ProtocolParser(Uart* uart)
    : uart_(uart), lastCmdIndex_(65535), buffer_(BUFFER_SIZE), state_(STATE_IDLE), frameStart_(0), cmdIndex_(0), contentLen_(0), totalFrameLen_(0) {}

void ProtocolParser::writeBuffer(const uint8_t* data, uint16_t len) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!buffer_.write(data, len)) {
        std::cerr << "缓冲区满，数据写入失败" << std::endl;
    }
}

int ProtocolParser::parseFrame() {
    std::lock_guard<std::mutex> lock(mutex_);

    // 状态机循环
    while (true) {
        switch (state_) {
            case STATE_IDLE:
                // 检查缓冲区数据是否足够
                if (buffer_.dataLength() < MIN_FRAME_LEN) {
                    // std::cerr << "缓冲区数据 < MIN_FRAME_LEN" << std::endl;
                    return -1;
                }
                state_ = STATE_FIND_HEADER;
                frameStart_ = 0;
                break;

            case STATE_FIND_HEADER:
                {
                    uint16_t dataLen = buffer_.dataLength();
                    bool foundHeader = false;
                    for (; frameStart_ <= dataLen - MIN_FRAME_LEN; ++frameStart_) {
                        if (buffer_[frameStart_] == HEAD_CMD_H && buffer_[frameStart_ + 1] == HEAD_CMD_L) {
                            foundHeader = true;
                            break;
                        }
                    }

                    if (!foundHeader) {
                        buffer_.consume(dataLen);  // 清除无效数据
                        state_ = STATE_IDLE;
                        // std::cerr << "无效数据" << std::endl;
                        return -1;
                    }
                    if (frameStart_ > 0) {
                        buffer_.consume(frameStart_);  // 跳过无效数据
                        state_ = STATE_IDLE;
                        std::cerr << "跳过无效数据" << std::endl;
                        return 0;  // 重新解析
                    }
                    state_ = STATE_PARSE_HEADER;
                }
                break;

            case STATE_PARSE_HEADER:
                // 解析帧头后字段
                cmdIndex_ = (buffer_[2] << 8) | buffer_[3];
                contentLen_ = (buffer_[4] << 8) | buffer_[5];
                totalFrameLen_ = contentLen_ + 8;  // 内容+帧头(2)+命令索引(2)+长度(2)+CRC(2)
                state_ = STATE_CHECK_FRAME_COMPLETE;
                break;

            case STATE_CHECK_FRAME_COMPLETE:
                // 检查帧是否完整
                if (buffer_.dataLength() < totalFrameLen_) {
                    // std::cerr << "数据不完整" << std::endl;
                    return -1;  // 数据不完整
                }
                state_ = STATE_VERIFY_CRC;
                break;

            case STATE_VERIFY_CRC:
                {
                    // 提取数据域（用于CRC校验）
                    std::vector<uint8_t> crcData(contentLen_ + 6);  // 帧头(2)+命令索引(2)+长度(2)+内容
                    for (uint16_t i = 0; i < 6; ++i) {
                        crcData[i] = buffer_[i];
                    }
                    for (uint16_t i = 0; i < contentLen_; ++i) {
                        crcData[i + 6] = buffer_[6 + i];
                    }

                    // 校验CRC
                    uint16_t receivedCrc = (buffer_[6 + contentLen_] << 8) | buffer_[7 + contentLen_];
                    uint16_t calculatedCrc = crcCalculator_.calculate(crcData.data(), crcData.size());
                    if (receivedCrc != calculatedCrc) {
                        buffer_.consume(2);  // 跳过错误帧头，继续解析
                        std::cerr << "CRC校验失败" << std::endl;
                        state_ = STATE_IDLE;
                        return 0;
                    }
                    state_ = STATE_CHECK_DUPLICATE;
                }
                break;

            case STATE_CHECK_DUPLICATE:
                // 去重检查
                // if (cmdIndex_ == lastCmdIndex_) {
                //     buffer_.consume(totalFrameLen_);
                //     std::cout << "重复数据，已跳过" << std::endl;
                //     state_ = STATE_IDLE;
                //     return 0;
                // }
                state_ = STATE_PARSE_JSON;
                break;

            case STATE_PARSE_JSON:
                // 解析JSON数据
                {
                    // 创建一个临时缓冲区来存储JSON数据
                    std::vector<uint8_t> jsonBuffer(contentLen_);
                    for (uint16_t i = 0; i < contentLen_ - 1; ++i) {
                        jsonBuffer[i] = buffer_[7 + i];
                    }
                    std::string jsonStr(reinterpret_cast<const char*>(jsonBuffer.data()), contentLen_);
                    if (!parseJson(jsonStr)) {
                        buffer_.consume(totalFrameLen_);
                        std::cerr << "JSON解析失败" << std::endl;
                        state_ = STATE_IDLE;
                        return 0;
                    }

                    // 转化为json打印
                    Json::StreamWriterBuilder writer;      //格式化
                    std::string json = Json::writeString(writer, jsonRoot_);
                    // std::cout << "解析的数据: " << std::endl << json << std::endl;
                    log_thread_safe(LOG_LEVEL_INFO, PROTOCOL_TAG, "recv : %s", json.c_str());
                    // std::cout << "cmdIndex: " << cmdIndex_ << std::endl;
                    log_thread_safe(LOG_LEVEL_INFO, PROTOCOL_TAG, "cmdIndex : %d", cmdIndex_);
                }
                state_ = STATE_BUILD_TASK;
                break;

            case STATE_BUILD_TASK:
                // 构造任务
                currentTask_.cmdIndex = cmdIndex_;
                currentTask_.subCommand = static_cast<SubCommand>(jsonRoot_["subCommand"].asInt());
                currentTask_.data = jsonRoot_["data"];
                state_ = STATE_CONSUME_DATA;
                break;

            case STATE_CONSUME_DATA:
                // 消费当前帧数据
                buffer_.consume(totalFrameLen_);
                lastCmdIndex_ = cmdIndex_;
                state_ = STATE_IDLE;
                std::cout << "解析成功" << std::endl;
                return 1;  // 解析成功

            case STATE_ERROR:
                // 错误状态处理
                buffer_.consume(1);  // 跳过一个字节
                std::cerr << "解析错误，跳过一个字节" << std::endl;
                state_ = STATE_IDLE;
                return 0;

            default:
                state_ = STATE_IDLE;
                std::cerr << "default set STATE_IDLE" << std::endl;
                return -1;
        }
    }
}

bool ProtocolParser::packData(std::vector<uint8_t>& pack, const std::string& jsonData, uint16_t cmdIndex) {
    // 验证JSON格式
    Json::Value temp;
    if (!JsonHelper::parse(jsonData, temp)) {
        std::cerr << "JSON格式错误: " << JsonHelper::getError() << std::endl;
        return false;
    }

    // 计算包大小
    uint16_t contentLen = jsonData.length() + 1;
    uint16_t totalLen = contentLen + 8;           // 帧头(2)+命令索引(2)+长度(2)+内容+CRC(2)
    pack.resize(totalLen);

    // 填充帧头
    pack[0] = HEAD_CMD_H;
    pack[1] = HEAD_CMD_L;

    // 填充命令索引
    pack[2] = (cmdIndex >> 8) & 0xFF;
    pack[3] = cmdIndex & 0xFF;

    // 填充长度
    pack[4] = (contentLen >> 8) & 0xFF;
    pack[5] = contentLen & 0xFF;

    pack[6] = 0x04;
    // 填充JSON数据
    std::memcpy(pack.data() + 7, jsonData.data(), contentLen - 1);

    // 计算并填充CRC
    uint16_t crc = crcCalculator_.calculate(pack.data(), contentLen + 6);
    pack[6 + contentLen] = (crc >> 8) & 0xFF;
    pack[7 + contentLen] = crc & 0xFF;

    return true;
}

Task ProtocolParser::getTask() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return currentTask_;
}

bool ProtocolParser::sendResponse(const Json::Value& root) {
    Json::StreamWriterBuilder writer;      //格式化
    std::string jsonStr = Json::writeString(writer, root);
    log_thread_safe(LOG_LEVEL_INFO, PROTOCOL_TAG, "test result: %s", jsonStr.c_str());
    std::vector<uint8_t> sendPack(jsonStr.length() + 9);
    if (!packData(sendPack, jsonStr, reportCmdIndex_++)) {
        std::cerr << "packData Error " << std::endl;
        log_thread_safe(LOG_LEVEL_ERROR, PROTOCOL_TAG, "packData Error");
        return false;
    }
    log_thread_safe(LOG_LEVEL_INFO, PROTOCOL_TAG, "send data length : %d", jsonStr.length() + 9);
    return uart_->sendData(sendPack.data(), sendPack.size()) == sendPack.size();
}

bool ProtocolParser::sendResponse(const Json::Value& root, int cmdIndex) {
    Json::StreamWriterBuilder writer;      //格式化
    std::string jsonStr = Json::writeString(writer, root);
    // std::cout << "回复应答：" << jsonStr << std::endl;
    log_thread_safe(LOG_LEVEL_INFO, PROTOCOL_TAG, "response data: %s", jsonStr.c_str());
    std::vector<uint8_t> sendPack(jsonStr.length() + 9);
    if (!packData(sendPack, jsonStr, cmdIndex)) {
        // std::cerr << "packData Error " << std::endl;
        log_thread_safe(LOG_LEVEL_ERROR, PROTOCOL_TAG, "packData Error");
        return false;
    }
    log_thread_safe(LOG_LEVEL_INFO, PROTOCOL_TAG, "response cmdIndex : %d, data length : %d", cmdIndex, jsonStr.length() + 9);

    // for (int i = 0; i < sendPack.size(); i++) {
    //     printf("%02X ", sendPack[i]);
    // }
    // printf("\n");
    return uart_->sendData(sendPack.data(), sendPack.size()) == sendPack.size();
}

bool ProtocolParser::sendResponse(const Json::Value& root, CmdType cmdType) {
    // Json::Value response = root;
    // response["cmdType"] = cmdType;

    // std::string jsonStr = JsonHelper::stringify(response);
    // std::vector<uint8_t> sendPack;

    // uint16_t cmdIndex = reportCmdIndex_++;
    // if (!packData(sendPack, jsonStr, cmdIndex)) {
    //     std::cerr << "packData Error " << std::endl;
    //     return false;
    // }
    // std::cout << "report jsonStr data: " << jsonStr.data() << std::endl;
    // return uart_.sendData(sendPack.data(), sendPack.size()) == sendPack.size();
    return false;
}

void ProtocolParser::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    buffer_.clear();
    jsonRoot_.clear();
    lastCmdIndex_ = 65535;
    currentTask_.subCommand = static_cast<SubCommand>(0);
    currentTask_.data = Json::Value();
    currentTask_.cmdIndex = 0;
}

bool ProtocolParser::parseJson(const std::string& jsonStr) {
    return JsonHelper::parse(jsonStr, jsonRoot_);
}

