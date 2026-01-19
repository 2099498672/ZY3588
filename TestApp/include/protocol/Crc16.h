#ifndef CRC16_H
#define CRC16_H

#include <cstdint>

namespace Crc16 {

/**
 * CRC16计算器类
 * 采用标准CRC-16/CCITT-FALSE算法
 * 多项式：x^16 + x^12 + x^5 + 1 (0x1021)
 * 初始值：0xFFFF
 * 结果异或值：0x0000
 */
class Calculator {
public:
    /**
     * 计算CRC16校验值
     * @param buffer 待校验数据缓冲区
     * @param length 数据长度（字节数）
     * @return 16位CRC校验结果
     */
    uint16_t calculate(const uint8_t* buffer, uint16_t length);
};

} // namespace Crc16

#endif // CRC16_H
