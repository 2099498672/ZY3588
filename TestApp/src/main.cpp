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

std::queue<std::shared_ptr<interrupt_flag>> interrupt_flags_queue_main;           
thread_pool work_thread_main(8);                                             

std::queue<std::shared_ptr<interrupt_flag>> interrupt_flags_queue_task;      
thread_pool work_thread_task(8);                                            

void test(std::shared_ptr<RkGenericBoard> Board) {
    // Board->getDdrSize();
    // Board->getEmmcSize();
    // Board->getPcieSize();

    // Board->cameraCapturePng(Board->cameraidToInfo.at("cam1").devicePath.c_str(), 
    //                         Board->cameraidToInfo.at("cam1").outputFilePath.c_str());

    // Board->cameraCapturePng(Board->cameraidToInfo.at("cam2").devicePath.c_str(), 
    //                         Board->cameraidToInfo.at("cam2").outputFilePath.c_str());

    // Board->cameraCapturePng(Board->cameraidToInfo.at("cam3").devicePath.c_str(), 
    //                         Board->cameraidToInfo.at("cam3").outputFilePath.c_str());

    // Board->cameraCapturePng(Board->cameraidToInfo.at("cam4").devicePath.c_str(), 
    //                         Board->cameraidToInfo.at("cam4").outputFilePath.c_str());

    // Board->cameraCapturePng(Board->cameraidToInfo.at("cam5").devicePath.c_str(), 
    //                         Board->cameraidToInfo.at("cam5").outputFilePath.c_str());

    //  bool ret = Board->serialTest("/dev/ttyS9");
    //  log_thread_safe(LOG_LEVEL_INFO, APP_TAG, "serial test /dev/ttyS9 result: %s", ret ? "success" : "failed");

    // if (Board->scanBluetoothDevices()) {
    //     if (Board->bluetoothScanResult.count > 0) {
    //         Board->printScanResult();
    //     }
    // }
}
void signal_handler(int signum);


int main() {
    char *val = getenv("BUILD_VER");
    log_thread_safe(LOG_LEVEL_INFO, APP_TAG, "固件版本: %s", val ? val : "未知");

    // 自动获取板卡名称逻辑在这里
    std::string detected_board_name = "ZY3588";
    std::shared_ptr<RkGenericBoard> Board = factory.create_board_v1(detected_board_name);
    if (!Board) {
        log_thread_safe(LOG_LEVEL_ERROR, APP_TAG, "无法创建板卡实例，程序退出");
        return 0;
    }
    log_thread_safe(LOG_LEVEL_INFO, APP_TAG, "创建板卡实例: %s", detected_board_name.c_str());
    Board->set_firmware_version(val ? val : "unknown");

    test(Board);

    ProtocolParser protocol(factory.uart);
    TaskQueue taskQueue;
    TaskHandler taskHandler(protocol);

    std::shared_ptr<interrupt_flag> recv_flag = std::make_shared<interrupt_flag>();
    interrupt_flags_queue_main.push(recv_flag);

    std::signal(SIGINT, signal_handler);
    log_thread_safe(LOG_LEVEL_INFO, APP_TAG, "已注册 SIGINT 信号处理函数，等待退出信号...");

    std::shared_ptr<interrupt_flag> task_handler_flag = 
        work_thread_main.submit_interruptible([&taskQueue, &taskHandler, &Board](interrupt_flag& flag) {
            log_thread_safe(LOG_LEVEL_INFO, APP_TAG, "任务处理线程启动");
            Task task;
            while (flag.is_stop_requested() == false) {
                if (taskQueue.tryPopFor(task, 10)) {
                    taskHandler.processTask(task, Board);
                }
            }
            log_thread_safe(LOG_LEVEL_INFO, APP_TAG, "任务处理线程退出");
        });
    interrupt_flags_queue_main.push(task_handler_flag);

    std::shared_ptr<interrupt_flag> sleep_for_main = std::make_shared<interrupt_flag>();
    interrupt_flags_queue_main.push(sleep_for_main);
    

    uint8_t buffer[1024];
    while(!recv_flag->is_stop_requested()) {
        ssize_t len = factory.uart->receiveData(buffer, sizeof(buffer));
        if (len > 0) {
            protocol.writeBuffer(buffer, static_cast<uint16_t>(len));
        } 

        int parseResult = protocol.parseFrame();
        if (parseResult == 1) {
            Task task = protocol.getTask();
            taskQueue.push(task);
        }  
    }

    while (!sleep_for_main->is_stop_requested()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    log_thread_safe(LOG_LEVEL_INFO, APP_TAG, "接收数据，解析线程退出");
    log_thread_safe(LOG_LEVEL_INFO, APP_TAG, "退出主循环，程序结束");
    return 0;
}

void signal_handler(int signal) {
    if (signal == SIGINT) {
        log_thread_safe(LOG_LEVEL_INFO, APP_TAG, " ctrl + c 按下，准备退出...");
        while (!interrupt_flags_queue_main.empty()) {
            std::shared_ptr<interrupt_flag> flag = interrupt_flags_queue_main.front();
            flag->request_stop();
            interrupt_flags_queue_main.pop();
        }

        while(!interrupt_flags_queue_task.empty()) {
            std::shared_ptr<interrupt_flag> flag = interrupt_flags_queue_task.front();
            flag->request_stop();
            interrupt_flags_queue_task.pop();
        }
    }
}

    // key detect task v3.0, need to move to TaskHandler
    // std::shared_ptr<interrupt_flag> key_task_interrupt_flag =
    //     work_thread_task.submit_interruptible([Board](interrupt_flag& flag) {
    //         log_thread_safe(LOG_LEVEL_INFO, APP_TAG, "key detect task thread started");
    //         Board->set_event_info_map({
    //             {"febd0030.pwm", {-1, "NULL", false}},
    //             {"adc-keys"    , {-1, "NULL", false}},
    //         });
    //         if (!Board->get_event_info()) {
    //             log_thread_safe(LOG_LEVEL_ERROR, APP_TAG, "key detect task thread exit due to get_event_info failure");
    //             return;
    //         }
    //         Board->print_event_info_map();

    //         while (!flag.is_stop_requested()) {
    //             int ret = Board->Key::wait_key_press(5);
    //             if (ret == -1) {
    //                 continue;
    //             } else if (ret == 0) {
    //                 continue;
    //             } else {
    //                 log_thread_safe(LOG_LEVEL_INFO, APP_TAG, "Detected key press - Key code: %d", ret);
    //             }
    //         }
    //         log_thread_safe(LOG_LEVEL_INFO, APP_TAG, "key detect task thread exited");
    //      });
    // interrupt_flags_queue_task.push(key_task_interrupt_flag);