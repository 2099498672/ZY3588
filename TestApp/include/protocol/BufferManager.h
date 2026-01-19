#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include "common/Constants.h"
#include "common/Types.h"
#include <cstdint>
#include <vector>

class BufferManager {
public:
    BufferManager(uint16_t size = BUFFER_SIZE);
    ~BufferManager() = default;

    // 写入数据到缓冲区
    bool write(const uint8_t* data, uint16_t len);

    // 读取指定长度数据（不移动读指针）
    int peek(uint8_t* dest, uint16_t len);

    // 移动读指针（消费数据）
    void consume(uint16_t len);

    // 获取当前数据长度
    uint16_t dataLength() const;

    // 清空缓冲区
    void clear();

    // 获取缓冲区指定位置数据（用于协议解析）
    uint8_t operator[](uint16_t index) const;

private:
    std::vector<uint8_t> buffer_;  // 缓冲区存储
    uint16_t readIdx_;             // 读指针
    uint16_t writeIdx_;            // 写指针
    uint16_t dataLen_;             // 当前数据长度
    const uint16_t bufferSize_;    // 缓冲区容量
};

#endif // BUFFER_MANAGER_H
