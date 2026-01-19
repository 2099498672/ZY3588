#ifndef __BOARD_FACTORY_H__
#define __BOARD_FACTORY_H__

#include <memory>
#include "hardware/RkGenericBoard.h"
#include "common/Constants.h"
#include "Uart.h"

#include "util/theradpoolv1/thread_pool.h"

// 工厂类
class BoardFactory {
public:
    // 根据板卡名称创建对应的板卡实例
    std::shared_ptr<RkGenericBoard> create_board_v1(const std::string& board_name);
    BOARD_NAME string_to_enum(const std::string& board_name_str);
    Uart* uart = nullptr;
    ~BoardFactory();
};

#endif