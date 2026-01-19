#include <thread>
#include <iostream>
#include <signal.h>
#include <atomic>
#include <csignal>
#include <vector>

#include "common/Constants.h"
#include "Uart.h"
#include "task/TaskQueue.h"
#include "task/TaskHandler.h"
#include "util/Timer.h"
#include "util/ZenityDialog.h"
#include "util/theradpoolv1/thread_pool.h"
#include "protocol/ProtocolParser.h"
#include "hardware/RkGenericBoard.h"
#include "project/BoardFactory.h"

const char *APP_TAG = "TestApp";    

BoardFactory factory;

std::vector<std::shared_ptr<interrupt_flag>> interrupt_flags_vector;           
thread_pool work_thread(8);                                             

std::queue<std::shared_ptr<interrupt_flag>> interrupt_flags_queue_task;      
thread_pool work_thread_task(8);                                            

void signal_handler(int signum);

int main() {
    // 自动获取板卡名称逻辑在这里
    std::string detected_board_name = "CM3588V2_CMD3588V2";
    std::shared_ptr<RkGenericBoard> Board = factory.create_board_v1(detected_board_name);
    if (!Board) {
        log_thread_safe(LOG_LEVEL_ERROR, APP_TAG, "无法创建板卡实例，程序退出");
        return 0;
    }
    log_thread_safe(LOG_LEVEL_INFO, APP_TAG, "创建板卡实例: %s", detected_board_name.c_str());

    std::signal(SIGINT, signal_handler);
    log_thread_safe(LOG_LEVEL_INFO, APP_TAG, "已注册 SIGINT 信号处理函数，等待退出信号...");

    std::shared_ptr<interrupt_flag> can2_back_task_flag =
        work_thread.submit_interruptible([board_ptr = Board](interrupt_flag& flag) {
            int can2_fd = board_ptr->open_can_device("can2");
            if (can2_fd < 0) {
                log_thread_safe(LOG_LEVEL_ERROR, APP_TAG, "打开 can2 设备失败，退出 can2 回应任务线程");
                return;
            }
            log_thread_safe(LOG_LEVEL_INFO, APP_TAG, "can2 回应任务线程启动,fd=%d", can2_fd);
            while (!flag.is_stop_requested()) {
                unsigned char buffer[32];
                bool ret = board_ptr->can_recv(can2_fd, buffer);
                if (ret) {
                    log_thread_safe(LOG_LEVEL_INFO, APP_TAG, "can2 收到数据:%s,准备回应", buffer);
                    board_ptr->can_send(can2_fd, 0x002, buffer);
                }
            }
            log_thread_safe(LOG_LEVEL_INFO, APP_TAG, "can2 回应任务线程退出");
        });
    interrupt_flags_vector.push_back(can2_back_task_flag);





    std::shared_ptr<interrupt_flag> sleep_for_main = std::make_shared<interrupt_flag>();
    interrupt_flags_vector.push_back(sleep_for_main);
    while (!sleep_for_main->is_stop_requested()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    log_thread_safe(LOG_LEVEL_INFO, APP_TAG, "退出主循环，程序结束");
    return 0;
}

void signal_handler(int signal) {
    if (signal == SIGINT) {
        log_thread_safe(LOG_LEVEL_INFO, APP_TAG, " ctrl + c 按下，准备退出...");
        for(std::shared_ptr<interrupt_flag> flag : interrupt_flags_vector) {
            flag->request_stop();
        }

        while(!interrupt_flags_queue_task.empty()) {
            std::shared_ptr<interrupt_flag> flag = interrupt_flags_queue_task.front();
            flag->request_stop();
            interrupt_flags_queue_task.pop();
        }
    }
}

// log_thread_safe(LOG_LEVEL_DEBUG, APP_TAG, "recv : %s", buffer);
// log_thread_safe(LOG_LEVEL_DEBUG, APP_TAG, "recv : %0x%x", buffer);

// std::cout << "开始解析" << std::endl;
// for (ssize_t i = 0; i < len; ++i) {
//     printf("%c", buffer[i]);
// }
// printf("\n");
// for (ssize_t i = 0; i < len; ++i) {
//     printf("0x%x ", buffer[i]);
// }
// printf("\n");