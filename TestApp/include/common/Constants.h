#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint>
#include <string>

// 版本信息
const std::string APP_VERSION = "CM3588-2025.8.15";

// 协议常量
const uint8_t HEAD_CMD_H = 0x3A;       // 帧头高位
const uint8_t HEAD_CMD_L = 0x3B;       // 帧头低位
const uint16_t BUFFER_SIZE = 4096;     // 缓冲区大小
const uint16_t MIN_FRAME_LEN = 8;      // 最小帧长度

// 命令类型
enum CmdType {
    CMD_REQUEST = 1,
    CMD_RESPONSE = 2
};

// 子命令类型
enum SubCommand {
    CMD_HANDSHAKE = 0x01,
    CMD_BEGIN_TEST = 0x02,
    CMD_OVER_TEST = 0x03,

    CMD_SIGNAL_TOBEMEASURED = 0x05,

    CMD_SIGNAL_TOBEMEASURED_RES = 0x07,

    CMD_SET_SYS_TIME = 0x0A,

    CMD_SIGNAL_EXEC = 0x0D,

    CMD_SIGNAL_EXEC_RES = 0x0E,
    CMD_SIGNAL_TOBEMEASURED_COMBINE = 0x0F,
    CMD_SIGNAL_TOBEMEASURED_COMBINE_RES = 0x10,

    CMD_SIGNAL_TEST_ITEM_RES = 6,
    CMD_OVER_TEST_ACK = 8,
    CMD_SN_MAC = 9,
    CMD_SN_MAC_ACK = 10
};

enum SignalExecOrder {
    SIGNAL_EXEC_ORDER_NULL = 0,
    SIGNAL_EXEC_ORDER_LN = 2,   
    SIGNAL_EXEC_ORDER_GPIO = 4,
    SIGNAL_EXEC_ORDER_MANUAL = 5,   // 手动测试命令

    // 手动测试命令

};

// 测试项类型
enum TestItem {
    NONE, 
    SERIAL,
    SERIAL_232,
    SERIAL_485, 
    CAN, 
    GPIO, 
    NET, 
    RTC, 
    STORAGE,
    USB,
    DDR,
    EMMC,
    TF,
    FACILITYUSB3_0,
    FACILITYUSB2_0,
    USB_PCIE,
    SWITCHS,
    CAMERA,
    TYPEC,
    WIFI, 
    BLUETOOTH, 
    BASE,
    MICROPHONE,
    AUDIO_AUTO_PALY,
    COMMON,
};

enum BOARD_NAME {
    BOARD_CM3588S2,
    BOARD_CM3588V2_CMD3588V2,
    BOARD_CM3576,
    BOARD_ZY3588,
    BOARD_UNKNOWN,
};

// 错误码
enum ErrorCode {
    ERROR_SUCCESS = 0,
    ERROR_INVALID_PARAM = 1,
    ERROR_HARDWARE_FAILURE = 2,
    ERROR_COMMUNICATION = 3,
    ERROR_TIMEOUT = 4,
    ERROR_UNKNOWN = 5
};

#endif // CONSTANTS_H
