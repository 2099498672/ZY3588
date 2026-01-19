#include "task/TaskHandler.h"
#include "hardware/TestInterface.h"
#include <iostream>
#include "util/theradpoolv1/thread_pool.h"
#include "util/ZenityDialog.h"
    
extern std::queue<std::shared_ptr<interrupt_flag>> interrupt_flags_queue_task; 
extern thread_pool work_thread_task;

ZenityDialog dialog;               // A graphical dialog box suitable for the Ubuntu Gnome desktop. It doesn't matter if it doesn't exist.

TaskHandler::TaskHandler(ProtocolParser& protocol) : protocol_(protocol), keyThreadStopFlag_(false) {

}

void TaskHandler::stop_all_tasks() {
    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "Stopping all ongoing test tasks...");
    while(!interrupt_flags_queue_task.empty()) {
        std::shared_ptr<interrupt_flag> flag = interrupt_flags_queue_task.front();
        flag->request_stop();
        interrupt_flags_queue_task.pop();
    }
}

void TaskHandler::processTask(const Task& task, std::shared_ptr<RkGenericBoard> Board) {
    switch (task.subCommand) {
        case CMD_HANDSHAKE:                         
            log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "CMD_HANDSHAKE : 0x01");
            handleHandshake(task);
            break;
        case CMD_BEGIN_TEST:                        
            log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "CMD_BEGIN_TEST : 0x02");
            stop_all_tasks();
            handleBeginTest(task);
            break;

        case CMD_OVER_TEST:                         
            log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "CMD_OVER_TEST : 0x03");
            stop_all_tasks();
            handleOverTest(task);
            break;

        case CMD_SIGNAL_TOBEMEASURED:              
            log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "CMD_SIGNAL_TOBEMEASURED : 0x05");
            handleSignalToBeMeasured(task);
            executeTestAndRespond(task, Board);
            break;

        case CMD_SET_SYS_TIME:
            log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "CMD_SET_SYS_TIME : 0x0A");
            handleSetSysTime(task);
            break;

        case CMD_SIGNAL_EXEC:                   
            std::cout << "CMD_SIGNAL_EXEC" << std::endl;
            log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "CMD_SIGNAL_EXEC : 0x0D");
            handleSignalExec(task);
            executeTestAndRespond(task, Board);
            break;

        case CMD_SIGNAL_TOBEMEASURED_COMBINE:     
            log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "CMD_SIGNAL_TOBEMEASURED_COMBINE : 0x0F");
            handleSignalToBeMeasuredCombine(task);
            executeTestAndRespond(task, Board);
            break;

        default:
            log_thread_safe(LOG_LEVEL_WARN, TaskHandlerTag, "unknown command: 0x%02X", task.subCommand);
            break;
    }
}

void TaskHandler::executeTestAndRespond(const Task& task, std::shared_ptr<RkGenericBoard> Board) {
    switch (task.subCommand) {
        case CMD_SIGNAL_TOBEMEASURED:                 // single to-be-measured command：0x05（5）
            switch(stringToTestItem(task.data["type"].asString())) {
                case STORAGE:                         // storage
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "execute storage test");
                    storage_test(task, Board);
                break;

                case SWITCHS:                         // switchs
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "execute switchs test");
                    switchs_test(task, Board);
                break;

                case SERIAL:                          // serial
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "execute serial test");
                    serial_test(task, Board);
                break;

                case RTC:                             // rtc
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "execute rtc test");
                    rtc_test(task, Board);
                break;

                case BLUETOOTH:                      // bluetooth
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "execute bluetooth test");
                    bluetooth_test(task, Board);
                break;

                case WIFI:                           // wifi
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "execute wifi test");
                    wifi_test(task, Board);
                break;

                case CAMERA:                         // camera
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "execute camera test");
                    camera_test(task, Board);
                break;

                case BASE:                          // base_info
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "execute base info test");
                    baseinfo_test(task, Board);
                break;

                case MICROPHONE:                    // microphone
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "execute microphone test");
                    microphone_test(task, Board);
                break;

                case COMMON:                            // common
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "execute common test");
                    common_test(task, Board);
                break;
            }
        break;

        case CMD_SIGNAL_EXEC:                         // single exec command：0x0D（13）
            switch(task.data["order"].asInt()) {
                case SIGNAL_EXEC_ORDER_LN:            // ln : 0x02(2)
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "execute LN test");
                    ln_test(task, Board);
                break;

                case SIGNAL_EXEC_ORDER_GPIO:          // gpio : 0x04(4)
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "execute GPIO test");
                    gpio_test(task, Board);
                break;

                case SIGNAL_EXEC_ORDER_MANUAL:           // manual : 0x05(5)
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "execute MANUAL test");
                    manual_test(task, Board);
                break;
            }
        break;

        case CMD_SIGNAL_TOBEMEASURED_COMBINE:         // single to-be-measured combine command：0x0F（15）
            switch(stringToTestItem(task.data["type"].asString())) {
                case NET:                              // net
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "execute net test");          
                    net_test(task, Board);
                break;

                case TYPEC:                            // typec
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "execute typec test");
                    typec_test(task, Board);
                break;
            }
        break;
    }
}

void TaskHandler::switchs_test(const Task& task, std::shared_ptr<RkGenericBoard> Board) {
    Json::Value response;
    Json::Value responseData;
    responseData = task.data;

    responseData["testCase"]["testResult"] = "OK";
    Json::Value& itemList = responseData["testCase"]["itemList"];
    response["result"] = "true";        

    std::map<int, std::string> keyMap;
    for (Json::Value& item : itemList) {                            // boot_11  power_12  Fixed format separation  Boot key-value name, 11 key-value
        if (item["enable"].asBool() == false) {                     // keys that are not enabled in the test items will be skipped during the detection.
            item["testResult"] = "SKIP";
            continue;
        }
        size_t underscorePos = item["type"].asString().find('_');                    // find out where is the underline is
        std::string keyName = item["type"].asString().substr(0, underscorePos);      // extract key name
        int keyCode = std::stoi(item["type"].asString().substr(underscorePos + 1));  // extract key code and convert to int
        keyMap[keyCode] = keyName;                                                   // store in map for later detection
    }

    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "-> switch test :  buttons pre-configured within the app are ....");
    for (const auto& pair : Board->Key::keyMap) {
        log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "-> switch test : button Code: %d, Button Name: %s", pair.first, pair.second.c_str());
    }

    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "-> switch test :  buttons to be detected are ....");
    for (const auto& pair : keyMap) {
        log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "-> switch test : button Code: %d, Button Name: %s", pair.first, pair.second.c_str());
        // if (Board->Key::keyMap.count(pair.first) == 0 ) {
        //     log_thread_safe(LOG_LEVEL_WARN, TaskHandlerTag, "--> switch test Warning: Key Code %d not found in board key mapping table", pair.first);
        // }
    }

    int detectCount = keyMap.size();
    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "-> switch test : Total number of buttons to be detected: %d", detectCount);

    // switch test version 1.0: using std::thread
    // std::thread keyThread ([this, Board = Board.get(), detectCount, keyMap, responseData]() mutable {
    //     int remainingCount = detectCount;
    //     Json::Value localResponseData = responseData;
    //     Json::Value localResponse;
    //     localResponse["result"] = "true";
        
    //     while(remainingCount > 0) {
    //         if (Board == nullptr) {
    //             std::cout << "Board指针无效" << std::endl;
    //             break;
    //         }
            
    //         int ret = Board->Key::waitForKeyAndIrPress();
    //         if (ret == -1) {
    //             // continue; // 超时或错误，继续等待
    //             std::cout << "按键检测错误或超时，线程退出" << std::endl;
    //             break;
    //         }

    //         if (Board->Key::keyMap.count(ret) && keyMap.count(ret) && Board->Key::keyMap[ret] == keyMap[ret]) {
    //             std::cout << "按键: " << Board->Key::keyMap[ret] << "按下，测试通过" << std::endl;
                
    //             Json::Value& itemList = localResponseData["testCase"]["itemList"];
    //             for (Json::Value& item : itemList) {
    //                 size_t underscorePos = item["type"].asString().find('_');
    //                 std::string keyName = item["type"].asString().substr(0, underscorePos);
    //                 int keyCode = std::stoi(item["type"].asString().substr(underscorePos + 1));
    //                 if (keyCode == ret) {
    //                     item["testResult"] = "OK";
    //                     break;
    //                 }
    //             }

    //             if (keyMap.count(ret)) {
    //                 // 从待测按键映射表中移除已测试按键
    //                 keyMap.erase(ret);
    //                 remainingCount--;
    //                 std::cout << "按键移除 " << Board->Key::keyMap[ret] << std::endl;
    //                 std::cout << "剩余待测按键数量: " << remainingCount << std::endl;
    //             }

    //             if (remainingCount == 0) {
    //                 std::cout << "所有按键测试完成，发送响应" << std::endl;
    //                 localResponse["cmdType"] = 1;
    //                 localResponse["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
    //                 localResponse["data"] = localResponseData;
    //                 this->protocol_.sendResponse(localResponse);
    //             }
    //         }
    //     }
        
    // });
    // keyThread.detach();

    // switch test version 2.0: using thread pool with interruptible task
    std::shared_ptr<interrupt_flag> keyThreadStopFlag =
        work_thread_task.submit_interruptible([this, Board, detectCount, keyMap, responseData](interrupt_flag& flag) mutable {
            // create key test thread, return interrupt_flag ptr

            log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "-> switch test :  switch test thread started");
            int remainingCount = detectCount;                             // remaining keys to test
            Json::Value localResponseData = responseData;          
            Json::Value localResponse;
            localResponse["result"] = "true";

            while(remainingCount > 0 && !flag.is_stop_requested()) {      // check if need to stop.
                if (Board == nullptr) {
                    log_thread_safe(LOG_LEVEL_ERROR, TaskHandlerTag, "-> switch test : Board pointer is invalid, exiting key test thread");
                    break;
                }
                
                int ret = Board->Key::waitForKeyPress(120);
                if (ret == -1) {
                    log_thread_safe(LOG_LEVEL_ERROR, TaskHandlerTag, "-> switch test : Key detection config error, exiting key test thread");
                    break;
                } else if (ret == 0) {
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "-> switch test : Key detection wait timeout, continue to wait for key press...");
                    continue;
                }

                if (Board->Key::keyMap.count(ret) && keyMap.count(ret) && Board->Key::keyMap[ret] == keyMap[ret]) {
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "-> switch test : Key: %s pressed, test passed", Board->Key::keyMap[ret].c_str());
                    
                    Json::Value& itemList = localResponseData["testCase"]["itemList"];
                    for (Json::Value& item : itemList) {
                        size_t underscorePos = item["type"].asString().find('_');
                        std::string keyName = item["type"].asString().substr(0, underscorePos);
                        int keyCode = std::stoi(item["type"].asString().substr(underscorePos + 1));
                        if (keyCode == ret) {
                            item["testResult"] = "OK";
                            break;
                        }
                    }

                    if (keyMap.count(ret)) {                                // remove tested key from to-be-tested key map
                        keyMap.erase(ret);  
                        remainingCount--;
                        log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "-> switch test : Key %s removed from to-be-tested key map", Board->Key::keyMap[ret].c_str());
                        log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "-> switch test : Remaining number of keys to be tested: %d", remainingCount);
                    }

                    if (remainingCount == 0) {
                        log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "-> switch test : All key tests completed, sending test result");
                        localResponse["cmdType"] = 1;
                        localResponse["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
                        localResponse["data"] = localResponseData;
                        this->protocol_.sendResponse(localResponse);
                    }
                }
            }
            log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "-> switch test : switch test thread exited");
    });
    interrupt_flags_queue_task.push(keyThreadStopFlag);
}

void TaskHandler::storage_test(const Task& task, std::shared_ptr<RkGenericBoard> Board) {
    Json::Value response;
    Json::Value responseData;
    responseData = task.data;

    Json::Value& store = responseData["testCase"]["store"];
    response["result"] = "true";

    int i = 0, j = 0, k = 0;
    Board->scan_usb_with_libusb();

    // for (const auto& size : Board->usbDiskSizeList) {
    //     log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "udisk size: %.2f GB", size);
    // }

    // for (const auto& info : Board->facilityUsbInfoList3_0) {
    //     log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "device VID: 0x%04x, PID: 0x%04x, type: %s", 
    //         info.vid, info.pid, info.usbType.c_str());
    // }

    // for (const auto& info : Board->facilityUsbInfoList2_0) {
    //     log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "device VID: 0x%04x, PID: 0x%04x, type: %s", 
    //         info.vid, info.pid, info.usbType.c_str());
    // }


    Board->lsusbGetVidPidInfo();


    for (Json::Value& item : store) {
        switch (stringToTestItem(item["name"].asString())) {
            case DDR: {
                if (item["enable"].asBool() == false) {
                    item["testResult"] = "SKIP";
                    break;
                }

                float size = Board->getDdrSize();
                char buffer[32];
                snprintf(buffer, sizeof(buffer), "%.2f", size);    // reserve two decimal fractions
                std::string strSize(buffer);
                item["testValue"] = strSize;
                if (size >= item["min"].asDouble() && size <= item["max"].asDouble()) {
                    item["testResult"] = "OK";
                } else {
                    item["testResult"] = "NG";
                    response["result"] = "false";
                }
            } break;

            case EMMC: {
                if (item["enable"].asBool() == false) {
                    item["testResult"] = "SKIP";
                    break;
                }

                double size = Board->getEmmcSize();
                char buffer[32];
                snprintf(buffer, sizeof(buffer), "%.2f", size);  // reserve two decimal fractions
                std::string strSize(buffer);
                item["testValue"] = strSize;
                if (size >= item["min"].asDouble() && size <= item["max"].asDouble()) {
                    item["testResult"] = "OK";
                } else {
                    item["testResult"] = "NG";
                    response["result"] = "false";
                }
            } break;

            case TF: {
                if (item["enable"].asBool() == false) {
                    item["testResult"] = "SKIP";
                    break;
                }

                double size = Board->getTfCardSize();
                char buffer[32];
                snprintf(buffer, sizeof(buffer), "%.2f", size);   // reserve two decimal fractions
                std::string strSize(buffer);
                item["testValue"] = strSize;
                if (size >= item["min"].asDouble() && size <= item["max"].asDouble()) {
                    item["testResult"] = "OK";
                } else {
                    item["testResult"] = "NG";
                    response["result"] = "false";
                }
            } break;

            case USB: {
                if (item["enable"].asBool() == false) {
                    item["testResult"] = "OK";
                    item["testValue"] = "SKIP";
                    break;
                }
        
                if (i <= Board->usbDiskCount) {
                    i++;
                    double size = Board->usbDiskSizeList[i];
                    char buffer[32];
                    snprintf(buffer, sizeof(buffer), "%.2f", size);     // reserve two decimal fractions
                    std::string strSize(buffer);
                    if (size >= item["min"].asDouble() && size <= item["max"].asDouble()) {
                        item["testValue"] = strSize;
                        item["testResult"] = "OK";
                    } else {
                        item["testValue"] = strSize;
                        item["testResult"] = "NG";
                        response["result"] = "false";
                    }
                } else {
                    item["testValue"] = 0;
                    item["testResult"] = "NG";
                    response["result"] = "false";
                    break;
                }
            } break;

            case USB_PCIE: {
                if (item["enable"].asBool() == false) {
                    item["testResult"] = "SKIP";
                    break;
                }

                double size = Board->getPcieSize();
                char buffer[32];
                snprintf(buffer, sizeof(buffer), "%.2f", size);     // reserve two decimal fractions
                std::string strSize(buffer);
                if (size >= item["min"].asDouble() && size <= item["max"].asDouble()) {
                    item["testValue"] = strSize;
                    item["testResult"] = "OK";
                } else {
                    item["testValue"] = strSize;
                    item["testResult"] = "NG";
                    response["result"] = "false";
                }
            } break;

            case FACILITYUSB3_0: {
                if (item["enable"].asBool() == false) {
                    item["testResult"] = "SKIP";
                    break;
                }

                for (auto & lsusbInfo : Board->lsusbFacilityUsbInfoList) {
                    if (lsusbInfo.vid == std::stoi(item["isVid"].asString()) &&
                        lsusbInfo.pid == std::stoi(item["isPid"].asString())) {
                            item["testResult"] = "OK";
                            break;
                    }
                }
            } break;

            case FACILITYUSB2_0: {
                if (item["enable"].asBool() == false) {
                    item["testResult"] = "SKIP";
                    break;
                }

                for (auto & lsusbInfo : Board->lsusbFacilityUsbInfoList) {
                    if (lsusbInfo.vid == std::stoi(item["isVid"].asString()) &&
                        lsusbInfo.pid == std::stoi(item["isPid"].asString())) {
                            item["testResult"] = "OK";
                            break;
                    }
                }
            } break;

            default:
                break;
        }
    }
    response["cmdType"] = 1;
    response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
    response["data"] = responseData;
    protocol_.sendResponse(response);
}

// void TaskHandler::serial_test(const Task& task, std::unique_ptr<RkGenericBoard>& Board) {
//     Json::Value response;
//     Json::Value responseData;
//     responseData = task.data;

//     std::thread serialThread ([this, Board = Board.get(), responseData]() mutable {
//         Json::Value response;
//         if (responseData["testCase"]["enable"].asBool() == false) {
//             return;
//         }

//         int serial485Count = 0;
//         std::vector<std::string> deviceList;
//         std::vector<bool> sendResult;
//         std::vector<bool> recvResult;

//         int serialCanCount = 0;
//         std::vector<std::string> canDeviceList;
//         std::vector<bool> canSendResult;
//         std::vector<bool> canRecvResult;

//         std::cout << "开始扫描串口设备..." << std::endl;
//         Json::Value& groupList = responseData["testCase"]["groupList"];

//         // 第一遍遍历：收集485设备并测试232设备
//         for (Json::ArrayIndex i = 0; i < groupList.size(); ++i) {
//             Json::Value& group = groupList[i];
            
//             // 获取 itemList 数组
//             Json::Value& itemList = group["itemList"];
            
//             // 遍历每个 item
//             for (Json::ArrayIndex j = 0; j < itemList.size(); ++j) {
//                 Json::Value& item = itemList[j];
//                 if (item["enable"].asBool() == false) {
//                     item["testResult"] = "SKIP";
//                     continue;
//                 }
//                 // 提取 serialPath
//                 if (item.isMember("serialPath") && item["serialPath"].isString()) {
//                     std::string serialPath = item["serialPath"].asString();
//                     std::string serialName = item["serialName"].asString();
//                     std::cout << "发现串口设备: " << serialPath << ", 名称: " << serialName << std::endl;
//                     std::cout << "进行分类或测试: " << serialPath << ", 名称: " << serialName << std::endl;

//                     if (strstr(serialName.c_str(), "232") != NULL) {
//                         std::cout << "测试串口: " << serialPath << std::endl;
//                         bool ret = Board->serialTest(serialPath.c_str());
//                         if (ret) {
//                             item["testResult"] = "OK";
//                         } else {
//                             item["testResult"] = "NG";
//                             response["result"] = "false";
//                         }
//                     } else if (strstr(serialName.c_str(), "485") != NULL) {
//                         deviceList.push_back(serialPath);
//                         serial485Count++;
//                     } else if (strstr(serialName.c_str(), "CAN") != NULL || strstr(serialName.c_str(), "can") != NULL) {
//                         canDeviceList.push_back(serialPath);
//                         serialCanCount++;
//                     }
//                 }
//             }
//         }

//         // 先打印出来看看
//         std::cout << "找到 " << serial485Count << " 个 485 串口设备 : " <<  deviceList.size() << std::endl;
//         for (size_t i = 0; i < deviceList.size(); ++i) {
//             std::cout << "485 设备: " << deviceList[i] << std::endl;
//         }

//         std::cout << "找到 " << serialCanCount << " 个 CAN 设备 : " <<  canDeviceList.size() << std::endl;
//         for (size_t i = 0; i < canDeviceList.size(); ++i) {
//             std::cout << "CAN 设备: " << canDeviceList[i] << std::endl;
//         }

//         // 测试485设备
//         if (!deviceList.empty() || !canDeviceList.empty()) {
//             Board->serial485TestRetry(deviceList, sendResult, recvResult);
//             // 打印结果
//             for (size_t i = 0; i < deviceList.size(); ++i) {
//                 std::cout << "485 设备: " << deviceList[i]
//                         << ", 发送: " << (sendResult[i] ? "成功" : "失败")
//                         << ", 接收: " << (recvResult[i] ? "成功" : "失败")
//                         << std::endl;
//             }

//             Board->serialCanTestRetry(canDeviceList, canSendResult, canRecvResult);
//             // 打印结果
//             for (size_t i = 0; i < canDeviceList.size(); ++i) {
//                 std::cout << "CAN 设备: " << canDeviceList[i]
//                         << ", 发送: " << (canSendResult[i] ? "成功" : "失败")
//                         << ", 接收: " << (canRecvResult[i] ? "成功" : "失败")
//                         << std::endl;
//             }

//             // 第二遍遍历：设置485设备的测试结果
//             int index = 0;
//             int canIndex = 0;
//             for (Json::ArrayIndex i = 0; i < groupList.size(); ++i) {
//                 Json::Value& group = groupList[i];
//                 Json::Value& itemList = group["itemList"];
                
//                 for (Json::ArrayIndex j = 0; j < itemList.size(); ++j) {
//                     Json::Value& item = itemList[j];
//                     if (item["enable"].asBool() == false) {
//                         continue;
//                     }
//                     if (item.isMember("serialPath") && item["serialPath"].isString()) {
//                         std::string serialName = item["serialName"].asString();
                        
//                         if (strstr(serialName.c_str(), "485") != NULL) {
//                             if (index < (int)deviceList.size()) {
//                                 if (sendResult[index] && recvResult[index]) {
//                                     item["testResult"] = "OK";
//                                 } else {
//                                     item["testResult"] = "NG";
//                                     response["result"] = "false";
//                                 }
//                                 index++;
//                             } else {
//                                 item["testResult"] = "NG";
//                                 response["result"] = "false";
//                             }
//                         } else if (strstr(serialName.c_str(), "CAN") != NULL || strstr(serialName.c_str(), "can") != NULL) {
//                             if (canIndex < (int)canDeviceList.size()) {
//                                 if (canSendResult[canIndex] && canRecvResult[canIndex]) {
//                                     item["testResult"] = "OK";
//                                 } else {
//                                     item["testResult"] = "NG";
//                                     response["result"] = "false";
//                                 }
//                                 canIndex++;
//                             } else {
//                                 item["testResult"] = "NG";
//                                 response["result"] = "false";
//                             }
//                         }
//                     }
//                 }
//             }
//         }

//         response["cmdType"] = 1;
//         response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
//         response["data"] = responseData;
//         protocol_.sendResponse(response);
//     });
//     serialThread.detach();
// }

void TaskHandler::serial_test(const Task& task, std::shared_ptr<RkGenericBoard> Board) {
    Json::Value response;
    Json::Value responseData;
    responseData = task.data;

    std::shared_ptr<interrupt_flag> serialThreadStopFlag =
        work_thread_task.submit_interruptible([this, Board, responseData](interrupt_flag& flag) mutable {
            Json::Value response;
            if (responseData["testCase"]["enable"].asBool() == false) {
                return;
            }

            Json::Value& groupList = responseData["testCase"]["groupList"];

            int serial485Count = 0;
            std::vector<std::string> deviceList;
            std::vector<bool> sendResult;
            std::vector<bool> recvResult;

            int serialCanCount = 0;
            std::vector<std::string> canDeviceList;
            std::vector<bool> canSendResult;
            std::vector<bool> canRecvResult;

            for (Json::ArrayIndex i = 0; i < groupList.size(); ++i) {
                Json::Value& group = groupList[i];

                // get itemList array
                Json::Value& itemList = group["itemList"];
                
                // iterate through each item
                for (Json::ArrayIndex j = 0; j < itemList.size(); ++j) {
                    Json::Value& item = itemList[j];
                    if (item["enable"].asBool() == false) {
                        item["testResult"] = "SKIP";
                        continue;
                    }

                    if (item.isMember("serialPath") && item["serialPath"].isString()) {
                        std::string serialPath = item["serialPath"].asString();
                        std::string serialName = item["serialName"].asString();
                        int mode = 0;
                        if (item["mode"].isInt()) {
                            mode = item["mode"].asInt();
                        } else if (item["mode"].isString()) {
                            mode = std::stoi(item["mode"].asString());
                        }

                        if (strstr(serialName.c_str(), "232") != NULL) {
                            bool ret = Board->serialTest(serialPath.c_str());
                            if (ret) {
                                item["testResult"] = "OK";
                            } else {
                                item["testResult"] = "NG";
                                response["result"] = "false";
                            }
                        } else if (strstr(serialName.c_str(), "485") != NULL && mode == 2) {
                            int fd = Board->openSerial(serialPath.c_str(), 115200);
                            if (fd < 0) {
                                log_thread_safe(LOG_LEVEL_ERROR, TaskHandlerTag, "open 485 device %s failed", serialPath.c_str());
                                item["testResult"] = "NG";
                                response["result"] = "false";
                            } else {
                                bool ret = Board->serial_485_test(fd, 20);
                                if (ret) {
                                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "485 device %s test success", serialPath.c_str());
                                    item["testResult"] = "OK";
                                } else {
                                    log_thread_safe(LOG_LEVEL_ERROR, TaskHandlerTag, "485 device %s test failed", serialPath.c_str());
                                    item["testResult"] = "NG";
                                    response["result"] = "false";
                                }
                                close(fd);
                            }
                        } else if (strstr(serialName.c_str(), "485") != NULL && mode == 0) {
                            log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "collect 485 %s mode %d", serialPath.c_str(), mode);
                            deviceList.push_back(serialPath);
                            serial485Count++;
                        } else if ((strstr(serialName.c_str(), "CAN") != NULL || strstr(serialName.c_str(), "can") != NULL) && mode == 2) {
                            log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "recv can %s mode %d", serialPath.c_str(), mode);
                            int can_fd = Board->open_can_device(serialPath.c_str());
                            if (can_fd < 0) {
                                log_thread_safe(LOG_LEVEL_ERROR, TaskHandlerTag, "open can device %s failed", serialPath.c_str());
                                item["testResult"] = "NG";
                                response["result"] = "false";
                            } else {
                                log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "%s fd : %d", serialPath.c_str(), can_fd);
                                bool ret = Board->can_test(can_fd, 3);
                                if (ret) {
                                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "can device %s test success", serialPath.c_str());
                                    item["testResult"] = "OK";
                                } else {
                                    log_thread_safe(LOG_LEVEL_ERROR, TaskHandlerTag, "can device %s test failed", serialPath.c_str());
                                    item["testResult"] = "NG";
                                    response["result"] = "false";
                                }
                                close(can_fd);
                            }
                        } else if ((strstr(serialName.c_str(), "CAN") != NULL || strstr(serialName.c_str(), "can") != NULL) && mode == 0) {
                            log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "collect can %s mode %d", serialPath.c_str(), mode);
                            canDeviceList.push_back(serialPath);
                            serialCanCount++;
                        }
                    }
                }
            }

            // test 485 and CAN devices collected before
            if (!deviceList.empty() || !canDeviceList.empty()) {
                Board->serial485TestRetry(deviceList, sendResult, recvResult);
                for (size_t i = 0; i < deviceList.size(); ++i) {
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "485 device: %s, send: %s, recv: %s",
                        deviceList[i].c_str(),
                        sendResult[i] ? "success" : "failed",
                        recvResult[i] ? "success" : "failed");
                }

                Board->serialCanTestRetry(canDeviceList, canSendResult, canRecvResult);
                for (size_t i = 0; i < canDeviceList.size(); ++i) {
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "CAN device: %s, send: %s, recv: %s",
                        canDeviceList[i].c_str(),
                        canSendResult[i] ? "success" : "failed",
                        canRecvResult[i] ? "success" : "failed");
                }

                // set test result for 485 and CAN devices
                int index = 0;
                int canIndex = 0;
                for (Json::ArrayIndex i = 0; i < groupList.size(); ++i) {
                    Json::Value& group = groupList[i];
                    Json::Value& itemList = group["itemList"];

                    for (Json::ArrayIndex j = 0; j < itemList.size(); ++j) {
                        Json::Value& item = itemList[j];
                        if (item["enable"].asBool() == false) {
                            continue;
                        }
                        if (item.isMember("serialPath") && item["serialPath"].isString()) {
                            std::string serialName = item["serialName"].asString();
                            int mode = 0;
                            if (item["mode"].isInt()) {
                                mode = item["mode"].asInt();
                            } else if (item["mode"].isString()) {
                                mode = std::stoi(item["mode"].asString());
                            }
                            
                            if (strstr(serialName.c_str(), "485") != NULL && mode == 0) {
                                if (index < (int)deviceList.size()) {
                                    if (sendResult[index] && recvResult[index]) {
                                        item["testResult"] = "OK";
                                    } else {
                                        item["testResult"] = "NG";
                                        response["result"] = "false";
                                    }
                                    index++;
                                } else {
                                    item["testResult"] = "NG";
                                    response["result"] = "false";
                                }
                            } else if ((strstr(serialName.c_str(), "CAN") != NULL || strstr(serialName.c_str(), "can") != NULL) && mode == 0) {
                                if (canIndex < (int)canDeviceList.size()) {
                                    if (canSendResult[canIndex] && canRecvResult[canIndex]) {
                                        item["testResult"] = "OK";
                                    } else {
                                        item["testResult"] = "NG";
                                        response["result"] = "false";
                                    }
                                    canIndex++;
                                } else {
                                    item["testResult"] = "NG";
                                    response["result"] = "false";
                                }
                            }
                        }
                    }
                }
            }

            response["cmdType"] = 1;
            response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
            response["data"] = responseData;
            protocol_.sendResponse(response);
        });
    interrupt_flags_queue_task.push(serialThreadStopFlag);

}

void TaskHandler::rtc_test(const Task& task, std::shared_ptr<RkGenericBoard> Board) {
    Json::Value response;
    Json::Value responseData;
    responseData = task.data;

    std::string rtc_time = responseData["testCase"]["timeValue"].asString();
    int year, month, day, hour, minute, second;
    struct rtc_time set_tm;
    struct rtc_time ret_time;
    char timeBuf[128];
    memset(timeBuf, 0, sizeof(timeBuf));
    sscanf(rtc_time.c_str(), "%d/%d/%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);

    set_tm.tm_year = year - 1900;
    set_tm.tm_mon = month - 1;
    set_tm.tm_mday = day;
    set_tm.tm_hour = hour;
    set_tm.tm_min = minute;
    set_tm.tm_sec = second;

    std::thread rtcThread([this, Board, set_tm, responseData]() mutable {
        Json::Value response;  // 在lambda内部定义response

        if (Board->setAndWait(set_tm, 2)) {
            char timeBuf[128];
            memset(timeBuf, 0, sizeof(timeBuf));
            sprintf(timeBuf, "%d/%d/%d %d:%d:%d", set_tm.tm_year + 1900, set_tm.tm_mon + 1, set_tm.tm_mday, set_tm.tm_hour, set_tm.tm_min, set_tm.tm_sec);
            std::string timeStr(timeBuf);
            responseData["testCase"]["testResult"] = "OK";
            responseData["testResult"] = "OK";
            responseData["testCase"]["testValue"] = timeStr;
            response["result"] = "true";
        } else {
            responseData["testCase"]["testResult"] = "NG";
            responseData["testResult"] = "NG";
            responseData["testCase"]["testValue"] = "NULL";
            response["result"] = "false";
        }
        
        response["cmdType"] = 1;
        response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
        response["data"] = responseData;
        protocol_.sendResponse(response);
    });

    rtcThread.detach();
}

void TaskHandler::ln_test(const Task& task, std::shared_ptr<RkGenericBoard> Board) {
    Json::Value response;
    Json::Value responseData;
    responseData = task.data;
    response["result"] = "true";

    std::string recv_ln = responseData["testOrder"]["ln"].asString();
    std::cout << "LN name to write and read: " << recv_ln << std::endl;

    //   "mac" : "0C63FC412AEE", 
    std::string mac = responseData["testOrder"]["mac"].asString();
    uint8_t lan_mac[6];
    sscanf(mac.c_str(), "%02x%02x%02x%02x%02x%02x", &lan_mac[0], &lan_mac[1], &lan_mac[2], &lan_mac[3], &lan_mac[4], &lan_mac[5]);

    if (Board->vendor_storage_write_mac(VENDOR_LAN_MAC_ID, lan_mac, 6) == 0) {
        std::cout << "LAN MAC write successful: " << mac << std::endl;
        uint8_t buffer[512];
        uint16_t len;
        int ret;
        len = sizeof(buffer);
        if (Board->vendor_storage_read(VENDOR_LAN_MAC_ID, buffer, &len) == 0) {
            std::cout << "LAN MAC read successful, data length: " << len << std::endl;
            char read_mac[18];
            memset(read_mac, 0, sizeof(read_mac));
            sprintf(read_mac, "%02X%02X%02X%02X%02X%02X", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
            std::string read_mac_string(read_mac);
            std::cout << "LAN MAC Data: " << read_mac_string << std::endl;
            if (read_mac_string == mac) {
                responseData["testOrder"]["result"] = "true";
                response["result"] = "true";
            } else {
                responseData["testOrder"]["result"] = "false";
                response["result"] = "false";
            }
        } else {
            responseData["testOrder"]["result"] = "false";
            response["result"] = "false";
        }


    } else {
        responseData["testOrder"]["result"] = "false";
        response["result"] = "false";
    }

    if (Board->vendor_storage_write_ln(recv_ln.c_str()) == 0) {
        uint8_t buffer[512];
        uint16_t len;
        int ret;
        len = sizeof(buffer);
        if (Board->vendor_storage_read(VENDOR_CUSTOM_ID_1, buffer, &len) == 0) {
            std::cout << "LN read successful, data length: " << len << std::endl;
            std::string read_ln((char*)buffer, len);
            std::cout << "LN Data: " << read_ln << std::endl;
            if (read_ln == recv_ln) {
                responseData["testOrder"]["result"] = "true";
                response["result"] = "true";
            } else {
                responseData["testOrder"]["result"] = "false";
                response["result"] = "false";
            }
        } else {
            responseData["testOrder"]["result"] = "false";
            response["result"] = "false";
        }
    } else {
        responseData["testOrder"]["result"] = "false";
        response["result"] = "false";
    }

    response["cmdType"] = 1;
    response["subCommand"] = CMD_SIGNAL_EXEC_RES;
    response["data"] = responseData;
    protocol_.sendResponse(response);
}

void TaskHandler::gpio_test(const Task& task, std::shared_ptr<RkGenericBoard> Board) {    // gpio task execute quickly, no need to add in thread pool
    Json::Value response;
    Json::Value responseData;
    responseData = task.data;
    response["result"] = "true";
    responseData["testResult"] = "OK";

    Json::Value &beanList = responseData["testOrder"]["beanList"];

    int direction;
    if (responseData["testOrder"].isMember("gpioType")) {
        direction = responseData["testOrder"]["gpioType"].asInt();
      
    } else {
        direction = 0;                               // default set output
    }

    LogInfo("TaskHandlerGPIO",  "direction: %d", direction);
    for (Json::ArrayIndex i = 0; i < beanList.size(); ++i) {
        int num = beanList[i]["num"].asInt();
        int group = beanList[i]["group"].asInt();
        int level = beanList[i]["level"].asInt();

        bool gpioResult;
        if (direction == 1) {
            gpioResult = Board->gpioGetValue(group * 32 + num, level);
            beanList[i]["result"] = gpioResult ? "true" : "false";
            beanList[i]["testValue"] = gpioResult ? "true" : "false";
            beanList[i]["testResult"] = gpioResult ? "OK" : "NG";
            if (!gpioResult) {
                response["result"] = "false";
                responseData["testResult"] = "NG";
            }
        } else {
            gpioResult = Board->gpioSetValue(group * 32 + num, level);
            beanList[i]["testValue"] = gpioResult ? "true" : "false";
            beanList[i]["testResult"] = gpioResult ? "OK" : "NG";
            if (!gpioResult) {
                response["result"] = "false";
                responseData["testResult"] = "NG";
            }
        }
    }

    if (direction == 1) {
        responseData["type"] = "gpio";
        responseData["testOrder"]["type"] = "1";
        response["cmdType"] = 1;
        response["subCommand"] = CMD_SIGNAL_EXEC_RES;
        response["data"] = responseData;
        protocol_.sendResponse(response);
    } else {
        response["cmdType"] = 1;
        response["subCommand"] = CMD_SIGNAL_EXEC_RES;
        response["data"] = responseData;
        protocol_.sendResponse(response);
    }
}

void TaskHandler::manual_test(const Task& task, std::shared_ptr<RkGenericBoard> Board) {   // manual task execute quickly, no need to add in thread pool
    Json::Value response;
    Json::Value response_data;
    response_data = task.data;
    response["result"] = "true";        

    std::string ledName = response_data["testOrder"]["artificialType"].asString();
    bool status = response_data["testOrder"]["state"].asBool();

    if (strstr(ledName.c_str(), "red") != NULL || strstr(ledName.c_str(), "Red") != NULL) {
        bool ret = Board->setRledStatus(status ? LED_ON : LED_OFF);
        if (ret) {
            response["result"] = "true";        
        } else {
            response["result"] = "false";
        }
    } else if (strstr(ledName.c_str(), "green") != NULL || strstr(ledName.c_str(), "Green") != NULL) {
        bool ret = Board->setGledStatus(status ? LED_ON : LED_OFF);
        if (ret) {
            response["result"] = "true";        
        } else {
            response["result"] = "false";
        }
    } else if (strstr(ledName.c_str(), "fan") != NULL || strstr(ledName.c_str(), "Fan") != NULL) {
        bool ret = Board->fanCtrl(status ? FAN_ON : FAN_OFF);
        if (ret) {
            response["result"] = "true";        
        } else {
            response["result"] = "false";
        }
    }

    response["cmdType"] = 1;
    response["subCommand"] = CMD_SIGNAL_EXEC_RES;
    response["data"] = response_data;
    protocol_.sendResponse(response);
}

void TaskHandler::net_test(const Task& task, std::shared_ptr<RkGenericBoard> Board) {
    Json::Value response;
    Json::Value responseData;
    responseData = task.data;
    response["result"] = "true";

    bool nic = false;
    bool nic2 = false;
    bool nic3 = false;
    

    char res[512];
    memset(res, 0, sizeof(res));
    if (responseData["testCase"]["groupData"]["testCase"].isMember("nic") && responseData["testCase"]["groupData"]["testCase"].isMember("ip")) {
        std::string nic = responseData["testCase"]["groupData"]["testCase"]["nic"].asString();
        std::string ip = responseData["testCase"]["groupData"]["testCase"]["ip"].asString();
        LogInfo("TaskHandler", "nic=%s, ip=%s", nic.c_str(), ip.c_str());

        int firstDot = ip.find('.');
        int secondDot = ip.find('.', firstDot + 1);
        std::string resultIp = ip.substr(0, secondDot);
        LogInfo("TaskHandler", "resultIp=%s", resultIp.c_str());
        char ipBuf[INET_ADDRSTRLEN];
        bool ret = Board->getInterfaceIp(nic.c_str(), ipBuf, sizeof(ipBuf));
        if (ret && strstr(ipBuf, resultIp.c_str()) != NULL) {
            char nicRes[128];
            memset(nicRes, 0, sizeof(nicRes));
            sprintf(nicRes, "%s:%s|", "DHCP1", ipBuf);
            strcat(res, nicRes);
            responseData["testCase"]["groupData"]["testCase"]["nicTestValue"] = std::string(ipBuf);
            responseData["testCase"]["groupData"]["testCase"]["nictestResult"] = "OK";
        } else {
            responseData["testCase"]["groupData"]["testCase"]["nicTestValue"] = "NULL";
            responseData["testCase"]["groupData"]["testCase"]["nictestResult"] = "NG";
            response["result"] = "false";
        }
    } 
    if(responseData["testCase"]["groupData"]["testCase"].isMember("nic2") && responseData["testCase"]["groupData"]["testCase"].isMember("ip2")) {
        std::string nic = responseData["testCase"]["groupData"]["testCase"]["nic2"].asString();
        std::string ip = responseData["testCase"]["groupData"]["testCase"]["ip2"].asString();
        LogInfo("TaskHandler", "nic=%s, ip=%s", nic.c_str(), ip.c_str());

        int firstDot = ip.find('.');
        int secondDot = ip.find('.', firstDot + 1);
        std::string resultIp = ip.substr(0, secondDot);
        LogInfo("TaskHandler", "resultIp=%s", resultIp.c_str());
        char ipBuf[INET_ADDRSTRLEN];
        bool ret = Board->getInterfaceIp(nic.c_str(), ipBuf, sizeof(ipBuf));
        if (ret && strstr(ipBuf, resultIp.c_str()) != NULL) {
            char nicRes[128];
            memset(nicRes, 0, sizeof(nicRes));
            sprintf(nicRes, "%s:%s|", "DHCP2", ipBuf);
            strcat(res, nicRes);
            responseData["testCase"]["groupData"]["testCase"]["nic2TestValue"] = std::string(ipBuf);
            responseData["testCase"]["groupData"]["testCase"]["nic2testResult"] = "OK";
        } else {
            responseData["testCase"]["groupData"]["testCase"]["nic2TestValue"] = "NULL";
            responseData["testCase"]["groupData"]["testCase"]["nic2testResult"] = "NG";
            response["result"] = "false";
        }
    } 
    if(responseData["testCase"]["groupData"]["testCase"].isMember("nic3") && responseData["testCase"]["groupData"]["testCase"].isMember("ip3")){
        std::string nic = responseData["testCase"]["groupData"]["testCase"]["nic3"].asString();
        std::string ip = responseData["testCase"]["groupData"]["testCase"]["ip3"].asString();
        char ipBuf[INET_ADDRSTRLEN];
        bool ret = Board->getInterfaceIp(nic.c_str(), ipBuf, sizeof(ipBuf));
        if (ret) {
            char nicRes[128];
            memset(nicRes, 0, sizeof(nicRes));
            sprintf(nicRes, "|%s:%s", "DHCP3", ipBuf);              
            strcat(res, nicRes);
            responseData["testCase"]["groupData"]["testCase"]["nic3TestValue"] = std::string(ipBuf);
            responseData["testCase"]["groupData"]["testCase"]["nic3testResult"] = "OK";
        } else {
            responseData["testCase"]["groupData"]["testCase"]["nic3TestValue"] = "NULL";
            responseData["testCase"]["groupData"]["testCase"]["nic3testResult"] = "NG";
            response["result"] = "false";
        }
    } 

    responseData["testCase"] = responseData["testCase"]["groupData"]["testCase"];
    responseData["testCase"]["testValue"] = std::string(res);
    responseData["testCase"]["testResult"] = response["result"].asString() == "true" ? "OK" : "NG";


    response["cmdType"] = 1;
    response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
    response["data"] = responseData;
    protocol_.sendResponse(response);
}

void TaskHandler::bluetooth_test(const Task& task, std::shared_ptr<RkGenericBoard> Board) {
    Json::Value response;
    Json::Value responseData;
    responseData = task.data;

    if (responseData["testCase"].isMember("enable") && responseData["testCase"]["enable"].asBool() == false) {
        return;
    }

    std::thread bluetoothThread([this, Board, responseData]() mutable {
        Json::Value response;  // 在lambda内部定义response
        response["result"] = "true";
        if (Board->scanBluetoothDevices()) {
            if (Board->bluetoothScanResult.count > 0) {
            Board->printScanResult();

            char rssiBuf[256];
            memset(rssiBuf, 0, sizeof(rssiBuf));
            sprintf(rssiBuf, "bluetoothMAC = %s;bluetoothSize = %d", Board->bluetoothScanResult.devices[0].macAddress, Board->bluetoothScanResult.count);
            responseData["testCase"]["rssi"]["testResult"] = "OK";
            responseData["testCase"]["rssi"]["testValue"] = std::string(rssiBuf);
            responseData["testResult"] = "OK";
            responseData["testCase"]["testValue"] = std::string(rssiBuf);
            // responseData["testCase"]["rssi"]["testDevicesCount"] = Board->bluetoothScanResult.count;
            // responseData["testCase"]["rssi"]["testDevicesMac"] = Board->bluetoothScanResult.devices[0].macAddress;
            responseData["testCase"]["testValue"] = std::string(rssiBuf);
            responseData["testCase"]["testResult"] = "OK";
            response["result"] = "true";
            }
        } else {
            responseData["testCase"]["rssi"]["testResult"] = "NG";
            responseData["testCase"]["rssi"]["testValue"] = -1;
            responseData["testCase"]["testResult"] = "NG";
            response["result"] = "false";
        }
        
        response["cmdType"] = 1;
        response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
        response["data"] = responseData;
        protocol_.sendResponse(response);
    });
    bluetoothThread.detach();
}

void TaskHandler::wifi_test(const Task& task, std::shared_ptr<RkGenericBoard> Board) {
    Json::Value response;
    Json::Value responseData;
    responseData = task.data;

    std::string ssid = responseData["testCase"]["wifiSsidList"][0]["ssid"].asString();
    int signalStrength = Board->wpaScanWifi(ssid.c_str());
    if (signalStrength != -1) {
        responseData["testCase"]["wifiSsidList"][0]["testValue"] = signalStrength;
        responseData["testCase"]["wifiSsidList"][0]["testResult"] = "OK";
        responseData["testCase"]["testResult"] = "OK";
        responseData["testCase"]["testValue"] = signalStrength;
        response["result"] = "true";
    } else {
        responseData["testCase"]["wifiSsidList"][0]["testValue"] = "NULL";
        responseData["testCase"]["wifiSsidList"][0]["testResult"] = "NG";
        response["result"] = "false";
    }
    response["cmdType"] = 1;
    response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
    response["data"] = responseData;
    protocol_.sendResponse(response);
}

void TaskHandler::typec_test(const Task& task, std::shared_ptr<RkGenericBoard> Board) {
    Json::Value response;
    Json::Value response_data;
    response_data = task.data;

    dialog.show("TYPE_C", "请插入TYPE-C设备");

    // type-c v1.0
    // std::thread typecThread([this, Board = Board.get(), responseData]() mutable {
    //     Json::Value response;  // 在lambda内部定义response
    //     response["result"] = "true";
    //     responseData["testCase"]["testResult"] = "OK";

    //     int count = 2;
    //     int first = -1;
    //     int timeOut = 60; // 60秒超时

    //     // Json::Value positive2_0;
    //     // Json::Value positive3_0;
    //     // Json::Value reverse2_0;
    //     // Json::Value reverse3_0;
    //     Board->scan_usb_with_libusb();

    //     bool testValue = false;  // 是否找到匹配的测试设备
    //     bool allTestValue = true; // 是否所有测试项都已测试完毕
    //     while(count) {
    //         int ret = Board->typeCTest("/sys/class/typec/port0/orientation");
    //         if ((ret == 1 || ret == 0) && count == 2) {
    //             sleep(1);
    //             Board->scan_usb_with_libusb();
    //             Json::Value& groupList = responseData["testCase"]["groupData"]["testCase"]["groupList"];
    //             for (Json::ArrayIndex i = 0; i < groupList.size(); ++i) {
    //                 if (groupList[i]["type"].asString() == "positive") {
    //                     for (Json::ArrayIndex j = 0; j < groupList[i]["itemList"].size(); ++j) {
                            
    //                         if (strstr(groupList[i]["itemList"][j]["name"].asString().c_str(), "3.0") != NULL) {
    //                             for (const auto& info : Board->facilityUsbInfoList3_0) {
    //                                 if (info.vid == std::stoi(groupList[i]["itemList"][j]["vid"].asString()) && 
    //                                     info.pid == std::stoi(groupList[i]["itemList"][j]["pid"].asString())) {
    //                                     char buf[256] = {0};
    //                                     sprintf(buf, "%d-%d", info.pid, info.vid);
    //                                     groupList[i]["itemList"][j]["testResult"] = "OK";
    //                                     groupList[i]["itemList"][j]["testValue"] = std::string(buf);
    //                                     std::cout << "找到匹配的3.0设备: VID=0x" << std::hex << info.vid 
    //                                               << ", PID=0x" << std::hex << info.pid 
    //                                               << ", 类型=" << info.usbType << std::dec << std::endl;
    //                                     testValue = true;
    //                                 } 
    //                             }
    //                             if (testValue == false) {
    //                                 allTestValue = false;
    //                                 groupList[i]["itemList"][j]["testResult"] = "NG";
    //                                 groupList[i]["itemList"][j]["testValue"] = "NG";
    //                                 responseData["testCase"]["testResult"] = "NG";
    //                                 std::cout << "未找到匹配的3.0设备: VID=0x" << std::hex << std::stoi(groupList[i]["itemList"][j]["vid"].asString()) 
    //                                           << ", PID=0x" << std::hex << std::stoi(groupList[i]["itemList"][j]["pid"].asString()) 
    //                                           << std::dec << std::endl;
    //                             } else {
    //                                 testValue = false; // 重置，继续下一个设备的匹配
    //                             }
                                
    //                         } else if (strstr(groupList[i]["itemList"][j]["name"].asString().c_str(), "2.0") != NULL) {
    //                             for (const auto& info : Board->facilityUsbInfoList2_0) {
    //                                 if (info.vid == std::stoi(groupList[i]["itemList"][j]["vid"].asString()) && 
    //                                     info.pid == std::stoi(groupList[i]["itemList"][j]["pid"].asString())) {
    //                                     char buf[256] = {0};
    //                                     sprintf(buf, "%d-%d", info.pid, info.vid);
    //                                     groupList[i]["itemList"][j]["testResult"] = "OK";
    //                                     groupList[i]["itemList"][j]["testValue"] = std::string(buf);
    //                                     std::cout << "找到匹配的2.0设备: VID=0x" << std::hex << info.vid 
    //                                               << ", PID=0x" << std::hex << info.pid 
    //                                               << ", 类型=" << info.usbType << std::dec << std::endl;
    //                                     testValue = true;
    //                                 } 
    //                             }

    //                             if (testValue == false) {
    //                                 allTestValue = false;
    //                                 groupList[i]["itemList"][j]["testResult"] = "NG";
    //                                 groupList[i]["itemList"][j]["testValue"] = "NG";
    //                                 responseData["testCase"]["testResult"] = "NG";
    //                                 std::cout << "未找到匹配的2.0设备: VID=0x" << std::hex << std::stoi(groupList[i]["itemList"][j]["vid"].asString()) 
    //                                           << ", PID=0x" << std::hex << std::stoi(groupList[i]["itemList"][j]["pid"].asString()) 
    //                                           << std::dec << std::endl;
    //                             } else {
    //                                 testValue = false; // 重置，继续下一个设备的匹配
    //                             }
    //                         }
    //                     }
    //                 }
    //             }


    //             first = ret;
    //             count--;
    //             std::cout << "Type-C 设备 第一次插入" << (ret == 1 ? "正插" : "反插") << std::endl;
    //             dialog.update("TYPE_C", "请翻转TYPE-C设备");
    //         }

    //         if (ret != -1 && ret != first && count == 1) {
    //             sleep(1);
    //             Board->scan_usb_with_libusb();
    //             for (const auto& info : Board->facilityUsbInfoList3_0) {
    //             std::cout << "设备 VID: 0x" << std::hex << info.vid 
    //                     << ", PID: 0x" << std::hex << info.pid 
    //                     << ", 类型: " << info.usbType << std::dec << std::endl;
    //             }

    //             for (const auto& info : Board->facilityUsbInfoList2_0) {
    //                 std::cout << "设备 VID: 0x" << std::hex << info.vid 
    //                         << ", PID: 0x" << std::hex << info.pid 
    //                         << ", 类型: " << info.usbType << std::dec << std::endl;
    //             }

    //             Json::Value& groupList = responseData["testCase"]["groupData"]["testCase"]["groupList"];
    //             for (Json::ArrayIndex i = 0; i < groupList.size(); ++i) {
    //                 if (groupList[i]["type"].asString() == "negative") {
    //                     for (Json::ArrayIndex j = 0; j < groupList[i]["itemList"].size(); ++j) {
                            
    //                         if (strstr(groupList[i]["itemList"][j]["name"].asString().c_str(), "3.0") != NULL) {
    //                             for (const auto& info : Board->facilityUsbInfoList3_0) {
    //                                 if (info.vid == std::stoi(groupList[i]["itemList"][j]["vid"].asString()) && 
    //                                     info.pid == std::stoi(groupList[i]["itemList"][j]["pid"].asString())) {
    //                                     char buf[256] = {0};
    //                                     sprintf(buf, "%d-%d", info.pid, info.vid);
    //                                     groupList[i]["itemList"][j]["testResult"] = "OK";
    //                                     groupList[i]["itemList"][j]["testValue"] = std::string(buf);
    //                                     std::cout << "找到匹配的3.0设备: VID=0x" << std::hex << info.vid 
    //                                               << ", PID=0x" << std::hex << info.pid 
    //                                               << ", 类型=" << info.usbType << std::dec << std::endl;

    //                                     testValue = true;
    //                                 } 
    //                             }

    //                             if (testValue == false) {
    //                                 allTestValue = false;
    //                                 groupList[i]["itemList"][j]["testResult"] = "NG";
    //                                 groupList[i]["itemList"][j]["testValue"] = "NG";
    //                                 responseData["testCase"]["testResult"] = "NG";
    //                                 std::cout << "未找到匹配的3.0设备: VID=0x" << std::hex << std::stoi(groupList[i]["itemList"][j]["vid"].asString()) 
    //                                           << ", PID=0x" << std::hex << std::stoi(groupList[i]["itemList"][j]["pid"].asString()) 
    //                                           << std::dec << std::endl;
    //                             } else {
    //                                 testValue = false; // 重置，继续下一个设备的匹配
    //                             }

    //                         } else if (strstr(groupList[i]["itemList"][j]["name"].asString().c_str(), "2.0") != NULL) {
    //                             for (const auto& info : Board->facilityUsbInfoList2_0) {
    //                                 if (info.vid == std::stoi(groupList[i]["itemList"][j]["vid"].asString()) && 
    //                                     info.pid == std::stoi(groupList[i]["itemList"][j]["pid"].asString())) {
    //                                     // groupList[i]["itemList"][j]["testResult"] = "OK";
    //                                     // groupList[i]["itemList"][j]["testValue"]["vid"] = info.vid;
    //                                     // groupList[i]["itemList"][j]["testValue"]["pid"] = info.pid;
    //                                     // groupList[i]["itemList"][j]["testValue"]["usbType"] = info.usbType;
    //                                     char buf[256] = {0};
    //                                     sprintf(buf, "%d-%d", info.pid, info.vid);
    //                                     groupList[i]["itemList"][j]["testResult"] = "OK";
    //                                     groupList[i]["itemList"][j]["testValue"] = std::string(buf);
    //                                     std::cout << "找到匹配的2.0设备: VID=0x" << std::hex << info.vid 
    //                                               << ", PID=0x" << std::hex << info.pid 
    //                                               << ", 类型=" << info.usbType << std::dec << std::endl;
    //                                     testValue = true;
    //                                 } 
    //                             }
    //                             if (testValue == false) {
    //                                 allTestValue = false;
    //                                 groupList[i]["itemList"][j]["testResult"] = "NG";
    //                                 groupList[i]["itemList"][j]["testValue"] = "NG";
    //                                 responseData["testCase"]["testResult"] = "NG";
    //                                 std::cout << "未找到匹配的2.0设备: VID=0x" << std::hex << std::stoi(groupList[i]["itemList"][j]["vid"].asString()) 
    //                                           << ", PID=0x" << std::hex << std::stoi(groupList[i]["itemList"][j]["pid"].asString()) 
    //                                           << std::dec << std::endl;
    //                             } else {
    //                                 testValue = false; // 重置，继续下一个设备的匹配
    //                             }
    //                         }
    //                     }
    //                 }
    //             }

    //             count--;
    //             std::cout << "Type-C 设备 第二次插入" << (ret == 1 ? "正插" : "反插") << std::endl;
    //             dialog.close();
           
    //         }
    
    //         if (timeOut-- == 0) {
    //             dialog.close();
    //             responseData["testCase"] = responseData["testCase"]["groupData"]["testCase"];

    //             std::cout << "Type-C 测试超时" << std::endl;
    //             responseData["testCase"]["testResult"] = "NG";
    //             responseData["testCase"]["testValue"] = "NULL";
    //             responseData["testResult"] = "NG";
    //             response["result"] = "false";
    //             break;
    //         }
    //         sleep(1);
    //     }
    //     responseData["testCase"] = responseData["testCase"]["groupData"]["testCase"];
    //     responseData["testCase"]["testResult"] = allTestValue ? "OK" : "NG";
    //     response["cmdType"] = 1;
    //     response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
    //     response["data"] = responseData;
    //     protocol_.sendResponse(response);
    // });
    // typecThread.detach();

    std::shared_ptr<interrupt_flag> typec_task_stop_flag = work_thread_task.submit_interruptible([this, Board, response_data](interrupt_flag& flag) mutable {
        Json::Value local_response;
        Json::Value local_response_data = response_data;
        local_response["result"] = "true";
        local_response_data["testCase"]["testResult"] = "OK";

        int count = 2;
        int first = -1;
        int timeOut = 60; // 60秒超时

        // Board->scan_usb_with_libusb();
        Board->lsusbGetVidPidInfo();

        bool testValue = false;  // 是否找到匹配的测试设备
        bool allTestValue = true; // 是否所有测试项都已测试完毕
        while(count && !flag.is_stop_requested()) {
            int ret = Board->typeCTest("/sys/class/typec/port0/orientation");
            if ((ret == 1 || ret == 0) && count == 2) {
                sleep(2);
                // Board->scan_usb_with_libusb();
                Board->lsusbGetVidPidInfo();
                Json::Value& groupList = local_response_data["testCase"]["groupData"]["testCase"]["groupList"];
                for (Json::ArrayIndex i = 0; i < groupList.size(); ++i) {
                    if (groupList[i]["type"].asString() == "positive") {
                        for (Json::ArrayIndex j = 0; j < groupList[i]["itemList"].size(); ++j) {
                            
                            if (strstr(groupList[i]["itemList"][j]["name"].asString().c_str(), "3.0") != NULL) {
                                for (const auto& info : Board->lsusbFacilityUsbInfoList) {
                                    if (info.vid == std::stoi(groupList[i]["itemList"][j]["vid"].asString()) && 
                                        info.pid == std::stoi(groupList[i]["itemList"][j]["pid"].asString())) {
                                        char buf[256] = {0};
                                        sprintf(buf, "%d-%d", info.pid, info.vid);
                                        groupList[i]["itemList"][j]["testResult"] = "OK";
                                        groupList[i]["itemList"][j]["testValue"] = std::string(buf);
                                        log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, 
                                            "type-c positive found matching 3.0 device: VID=0x%X, PID=0x%X", 
                                            info.vid, info.pid);
                                        testValue = true;
                                    } 
                                }
                                if (testValue == false) {
                                    allTestValue = false;
                                    groupList[i]["itemList"][j]["testResult"] = "NG";
                                    groupList[i]["itemList"][j]["testValue"] = "NG";
                                    local_response_data["testCase"]["testResult"] = "NG";
                                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, 
                                        "type-c positive no found matching 3.0 device: VID=0x%X, PID=0x%X", 
                                        std::stoi(groupList[i]["itemList"][j]["vid"].asString()), 
                                        std::stoi(groupList[i]["itemList"][j]["pid"].asString()));
                                } else {
                                    testValue = false; // 重置，继续下一个设备的匹配
                                }
                            } else if (strstr(groupList[i]["itemList"][j]["name"].asString().c_str(), "2.0") != NULL) {
                                for (const auto& info : Board->lsusbFacilityUsbInfoList) {
                                    if (info.vid == std::stoi(groupList[i]["itemList"][j]["vid"].asString()) && 
                                        info.pid == std::stoi(groupList[i]["itemList"][j]["pid"].asString())) {
                                        char buf[256] = {0};
                                        sprintf(buf, "%d-%d", info.pid, info.vid);
                                        groupList[i]["itemList"][j]["testResult"] = "OK";
                                        groupList[i]["itemList"][j]["testValue"] = std::string(buf);
                                        log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, 
                                            "type-c positive found matching 2.0 device: VID=0x%X, PID=0x%X", 
                                            info.vid, info.pid);
                                        testValue = true;
                                    } 
                                }

                                if (testValue == false) {
                                    allTestValue = false;
                                    groupList[i]["itemList"][j]["testResult"] = "NG";
                                    groupList[i]["itemList"][j]["testValue"] = "NG";
                                    local_response_data["testCase"]["testResult"] = "NG";
                                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, 
                                        "type-c positive no found matching 2.0 device: VID=0x%X, PID=0x%X", 
                                        std::stoi(groupList[i]["itemList"][j]["vid"].asString()), 
                                        std::stoi(groupList[i]["itemList"][j]["pid"].asString()));
                                        
                                } else {
                                    testValue = false; // 重置，继续下一个设备的匹配
                                }
                            }
                        }
                    }
                }

                first = ret;
                count--;
                log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "type-c device first inserted %s", (ret == 1 ? "normal" : "reverse"));
                dialog.update("TYPE_C", "请翻转TYPE-C设备");
            }

            if (ret != -1 && ret != first && count == 1) {
                sleep(2);
                // Board->scan_usb_with_libusb();        
                Board->lsusbGetVidPidInfo();

                Json::Value& groupList = local_response_data["testCase"]["groupData"]["testCase"]["groupList"];
                for (Json::ArrayIndex i = 0; i < groupList.size(); ++i) {
                    if (groupList[i]["type"].asString() == "negative") {
                        for (Json::ArrayIndex j = 0; j < groupList[i]["itemList"].size(); ++j) {
                            
                            if (strstr(groupList[i]["itemList"][j]["name"].asString().c_str(), "3.0") != NULL) {
                                for (const auto& info : Board->lsusbFacilityUsbInfoList) {
                                    if (info.vid == std::stoi(groupList[i]["itemList"][j]["vid"].asString()) && 
                                        info.pid == std::stoi(groupList[i]["itemList"][j]["pid"].asString())) {
                                        char buf[256] = {0};
                                        sprintf(buf, "%d-%d", info.pid, info.vid);
                                        groupList[i]["itemList"][j]["testResult"] = "OK";
                                        groupList[i]["itemList"][j]["testValue"] = std::string(buf);
                                        log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, 
                                            "type-c negative found matching 3.0 device: VID=0x%X, PID=0x%X", 
                                            info.vid, info.pid);
                                        testValue = true;
                                    } 
                                }

                                if (testValue == false) {
                                    allTestValue = false;
                                    groupList[i]["itemList"][j]["testResult"] = "NG";
                                    groupList[i]["itemList"][j]["testValue"] = "NG";
                                    local_response_data["testCase"]["testResult"] = "NG";
                                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, 
                                        "type-c negative no found matching 3.0 device: VID=0x%X, PID=0x%X", 
                                        std::stoi(groupList[i]["itemList"][j]["vid"].asString()), 
                                        std::stoi(groupList[i]["itemList"][j]["pid"].asString()));
                                } else {
                                    testValue = false; // 重置，继续下一个设备的匹配
                                }

                            } else if (strstr(groupList[i]["itemList"][j]["name"].asString().c_str(), "2.0") != NULL) {
                                for (const auto& info : Board->lsusbFacilityUsbInfoList) {
                                    if (info.vid == std::stoi(groupList[i]["itemList"][j]["vid"].asString()) && 
                                        info.pid == std::stoi(groupList[i]["itemList"][j]["pid"].asString())) {
                                        char buf[256] = {0};
                                        sprintf(buf, "%d-%d", info.pid, info.vid);
                                        groupList[i]["itemList"][j]["testResult"] = "OK";
                                        groupList[i]["itemList"][j]["testValue"] = std::string(buf);
                                        log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, 
                                            "type-c negative found matching 2.0 device: VID=0x%X, PID=0x%X", 
                                            info.vid, info.pid);
                                        testValue = true;
                                    } 
                                }
                                if (testValue == false) {
                                    allTestValue = false;
                                    groupList[i]["itemList"][j]["testResult"] = "NG";
                                    groupList[i]["itemList"][j]["testValue"] = "NG";
                                    local_response_data["testCase"]["testResult"] = "NG";
                                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag,
                                        "type-c negative no found matching 2.0 device: VID=0x%X, PID=0x%X",
                                        std::stoi(groupList[i]["itemList"][j]["vid"].asString()),
                                        std::stoi(groupList[i]["itemList"][j]["pid"].asString()));
                                } else {
                                    testValue = false; // 重置，继续下一个设备的匹配
                                }
                            }
                        }
                    }
                }

                count--;
                log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "type-c device second inserted %s", (ret == 1 ? "normal" : "reverse"));
                dialog.close();
            }
    
            if (timeOut-- == 0) {
                dialog.close();
                local_response_data["testCase"] = local_response_data["testCase"]["groupData"]["testCase"];

                std::cout << "Type-C 测试超时" << std::endl;
                local_response_data["testCase"]["testResult"] = "NG";
                local_response_data["testCase"]["testValue"] = "NULL";
                local_response_data["testResult"] = "NG";
                local_response["result"] = "false";
                break;
            }
            sleep(1);
        }
        local_response_data["testCase"] = local_response_data["testCase"]["groupData"]["testCase"];
        local_response_data["testCase"]["testResult"] = allTestValue ? "OK" : "NG";
        local_response["cmdType"] = 1;
        local_response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
        local_response["data"] = local_response_data;
        protocol_.sendResponse(local_response);
    });
    interrupt_flags_queue_task.push(typec_task_stop_flag);
}

void TaskHandler::camera_test(const Task& task, std::shared_ptr<RkGenericBoard> Board) {
    Json::Value response;
    Json::Value response_data;
    response_data = task.data;

    if (response_data["testCase"].isMember("enable") && response_data["testCase"]["enable"].asBool() == false) {
        return;
    }

    // camera test v1
    // if (responseData["testCase"].isMember("cameraId") && responseData["testCase"]["cameraId"].asInt() == 0) {
    //     std::string picturePixel;
    //     int ret = Board->cameraTestCam0(picturePixel);
    //     if (ret) {
    //         responseData["testCase"]["testResult"] = "OK";
    //         responseData["testCase"]["testValue"] = "0";
    //         responseData["testResult"] = "OK";
    //         responseData["testValue"] = picturePixel;
    //         response["result"] = "true";
    //     } else {
    //         responseData["testCase"]["testResult"] = "NG";
    //         responseData["testCase"]["testValue"] = "1";
    //         responseData["testResult"] = "NG";
    //         response["result"] = "false";
    //     }
    //     response["cmdType"] = 1;
    //     response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
    //     response["data"] = responseData;
    //     protocol_.sendResponse(response);
    // } else if (responseData["testCase"].isMember("cameraId") && responseData["testCase"]["cameraId"].asInt() == 1) {
    //     std::string picturePixel;
    //     int ret = Board->cameraTestCam1(picturePixel);
    //     if (ret) {
    //         responseData["testCase"]["testResult"] = "OK";
    //         responseData["testCase"]["testValue"] = "0";
    //         responseData["testResult"] = "OK";
    //         responseData["testValue"] = picturePixel;
    //         response["result"] = "true";
    //     } else {
    //         responseData["testCase"]["testResult"] = "NG";
    //         responseData["testCase"]["testValue"] = "1";
    //         responseData["testResult"] = "NG";
    //         response["result"] = "false";
    //     }
    //     response["cmdType"] = 1;
    //     response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
    //     response["data"] = responseData;
    //     protocol_.sendResponse(response);
    // } else if (responseData["testCase"].isMember("cameraId") && responseData["testCase"]["cameraId"].asInt() == 2) {
    //     // std::string picturePixel;
    //     // int ret = Board->cameraTestCam1(picturePixel);
    //     // if (ret) {
    //     //     responseData["testCase"]["testResult"] = "OK";
    //     //     responseData["testCase"]["testValue"] = "0";
    //     //     responseData["testResult"] = "OK";
    //     //     responseData["testValue"] = picturePixel;
    //     //     response["result"] = "true";
    //     // } else {
    //     //     responseData["testCase"]["testResult"] = "NG";
    //     //     responseData["testCase"]["testValue"] = "1";
    //     //     responseData["testResult"] = "NG";
    //     //     response["result"] = "false";
    //     // }
    //     // response["cmdType"] = 1;
    //     // response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
    //     // response["data"] = responseData;
    //     // protocol_.sendResponse(response);
    // }

    // camera test v2
    // std::thread camThread([this, Board = Board.get(), responseData]() mutable {
    //     Json::Value response;  // 在lambda内部定义response
    //     response["result"] = "true";
    //     if (responseData["testCase"].isMember("cameraId")) {
    //         std::string cameraId = responseData["testCase"]["cameraId"].asString();
    //         std::string cameraName = "cam" + cameraId;
    //         if (Board->cameraidToInfo.find(cameraName) == Board->cameraidToInfo.end()) {
    //             responseData["testCase"]["testResult"] = "NG";
    //             responseData["testResult"] = "NG";
    //             response["result"] = "false";
    //         } else {
    //             bool ret = Board->cameraCapturePng(Board->cameraidToInfo.at(cameraName).devicePath.c_str(), 
    //                                                 Board->cameraidToInfo.at(cameraName).outputFilePath.c_str());
                
    //             if (ret) {
    //                 responseData["testCase"]["testResult"] = "OK";
    //                 responseData["testResult"] = "OK";
    //                 responseData["testCase"]["testValue"] = "0";
    //                 response["result"] = "true";
    //             } else {
    //                 responseData["testCase"]["testResult"] = "NG";
    //                 responseData["testResult"] = "NG";
    //                 responseData["testCase"]["testValue"] = "1";
    //                 response["result"] = "false";
    //             }
    //         }
    //         response["cmdType"] = 1;
    //         response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
    //         response["data"] = responseData;
    //         protocol_.sendResponse(response);
    //     }
    // });
    // camThread.detach();

    std::shared_ptr<interrupt_flag> camera_task_stop_flag =
        work_thread_task.submit_interruptible([this, Board, response_data](interrupt_flag& flag) mutable {
            Json::Value local_response;
            Json::Value local_response_data = response_data;
            local_response["result"] = "true";
            if (local_response_data["testCase"].isMember("cameraId")) {
                std::string cameraId = local_response_data["testCase"]["cameraId"].asString();
                std::string cameraName = "cam" + cameraId;
                if (Board->cameraidToInfo.find(cameraName) == Board->cameraidToInfo.end()) {
                    local_response_data["testCase"]["testResult"] = "NG";
                    local_response_data["testResult"] = "NG";
                    local_response["result"] = "false";
                } else {
                    bool ret = Board->cameraCapturePng(Board->cameraidToInfo.at(cameraName).devicePath.c_str(), 
                                                        Board->cameraidToInfo.at(cameraName).outputFilePath.c_str());
                
                    if (ret) {
                        local_response_data["testCase"]["testResult"] = "OK";
                        local_response_data["testResult"] = "OK";
                        local_response_data["testCase"]["testValue"] = "0";
                        local_response["result"] = "true";
                    } else {
                        local_response_data["testCase"]["testResult"] = "NG";
                        local_response_data["testResult"] = "NG";
                        local_response_data["testCase"]["testValue"] = "1";
                        local_response["result"] = "false";
                    }
                }
                local_response["cmdType"] = 1;
                local_response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
                local_response["data"] = local_response_data;
                protocol_.sendResponse(local_response);
            }
        });
    interrupt_flags_queue_task.push(camera_task_stop_flag);
}

// read base info: firmware version, hwid, ln, mac, app version, do not consume time, so no need to create a new thread 
void TaskHandler::baseinfo_test(const Task& task, std::shared_ptr<RkGenericBoard> Board) {
    Json::Value response;
    Json::Value response_data;
    response_data = task.data;

    if (response_data["testCase"]["firmwareVersion"]["enable"].asBool() == true) {            // get firmware version
        std::string firmware_version;
        // bool ret = Board->get_firmware_version(firmware_version);
        // response_data["testCase"]["firmwareVersion"]["testValue"] = ret ? firmware_version : "NULL"; 
        response_data["testCase"]["firmwareVersion"]["testValue"] = Board->get_firmware_version();
    }

    if (response_data["testCase"]["hwid"]["enable"].asBool() == true) {                      // get hwid
        std::string hwid;
        int ret = Board->get_hwid(hwid);
        response_data["testCase"]["hwid"]["testValue"] = ret ? hwid : "NULL";
    }

    if (response_data["testCase"]["ln"]["enable"].asBool() == true) {                        // get ln
        char buffer[512];
        uint16_t len = sizeof(buffer);
        if (Board->vendor_storage_read(VENDOR_CUSTOM_ID_1, (uint8_t*)buffer, &len) == 0) {
            std::string read_ln((char*)buffer, len);
            log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "read ln : %s", read_ln.c_str());
            response_data["testCase"]["ln"]["testValue"] = read_ln;
        } else {
            response_data["testCase"]["ln"]["testValue"] = "NULL";
        }
    }

    if (response_data["testCase"]["mac"]["enable"].asBool() == true) {                      // get mac
        uint8_t buffer[512];
        uint16_t len = sizeof(buffer);
        if (Board->vendor_storage_read(VENDOR_LAN_MAC_ID, buffer, &len) == 0) {
            char read_mac[18];
            memset(read_mac, 0, sizeof(read_mac));
            sprintf(read_mac, "%02X:%02X:%02X:%02X:%02X:%02X", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
            std::string read_mac_string(read_mac);
            log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "read mac : %s", read_mac_string.c_str());
            response_data["testCase"]["mac"]["testValue"] = read_mac_string;
        } else {
            response_data["testCase"]["mac"]["testValue"] = "NULL";
        }
    }

    if (response_data["testCase"]["testAppVersion"]["enable"].asBool() == true) {              // get app version
        std::string app_version;
        int ret = Board->get_app_version(app_version);
        response_data["testCase"]["testAppVersion"]["testValue"] = ret ? app_version : "NULL";
    }

    response["cmdType"] = 1;
    response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
    response["data"] = response_data;
    protocol_.sendResponse(response);
}

void TaskHandler::microphone_test(const Task& task, std::shared_ptr<RkGenericBoard> Board) {
    Json::Value response;
    Json::Value response_data;
    response_data = task.data;

    if (response_data["testCase"].isMember("enable") && response_data["testCase"]["enable"].asBool() == false) {
        return;
    }

    std::shared_ptr<interrupt_flag> mic_task_stop_flag =
        work_thread_task.submit_interruptible([this, Board, response_data](interrupt_flag& flag) mutable {
            Json::Value local_response;
            Json::Value local_response_data = response_data;
            local_response["result"] = "true";

            int ret = Board->record_and_compare_audio_v2(3);               // please read record_and_compare_audio_v2 function for more details
            if (ret == 0x03) {
                local_response_data["testCase"]["testResult"] = "OK";
                local_response_data["testResult"] = "OK";
                local_response["result"] = "true";
            } else if (ret == 0x02) {
                local_response_data["testCase"]["testResult"] = "NG";
                local_response_data["testResult"] = "NG";
                local_response_data["testCase"]["desc"] = "right filed";
                local_response["result"] = "false";
            } else if (ret == 0x01) {
                local_response_data["testCase"]["testResult"] = "NG";
                local_response_data["testResult"] = "NG";
                local_response_data["testCase"]["desc"] = "left filed";
                local_response["result"] = "false";
            } else {
                local_response_data["testCase"]["testResult"] = "NG";
                local_response_data["testResult"] = "NG";
                local_response_data["testCase"]["desc"] = "left and right filed";
                local_response["result"] = "false";
            }

            local_response["cmdType"] = 1;
            local_response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
            local_response["data"] = local_response_data;
            protocol_.sendResponse(local_response);

            log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "Microphone test thread exit.");
        });
    interrupt_flags_queue_task.push(mic_task_stop_flag);
}

// common test items are not consume time, but in the future, the number of test items may increase. So create a new thread to handle common test items
void TaskHandler::common_test(const Task& task, std::shared_ptr<RkGenericBoard> Board) {
    Json::Value response;
    Json::Value response_data;
    response_data = task.data;

    if (response_data["testCase"].isMember("enable") && response_data["testCase"]["enable"].asBool() == false) {
        return;
    }
   
    std::shared_ptr<interrupt_flag> common_task_stop_flag =
        work_thread_task.submit_interruptible([this, Board, response_data](interrupt_flag& flag) mutable {
            Json::Value local_response;
            Json::Value local_response_data = response_data;
            local_response["result"] = "true";

            for (Json::ArrayIndex i = 0; i < local_response_data["testCase"]["rangeCaseList"].size(); ++i) {
                if (flag.is_stop_requested()) {
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "Common test thread exit.");
                    break;
                }

                if (local_response_data["testCase"]["rangeCaseList"][i]["enable"].asBool() == false) {
                    continue;
                }

                std::string common_name = local_response_data["testCase"]["rangeCaseList"][i]["name"].asString();
                int max = local_response_data["testCase"]["rangeCaseList"][i]["max"].asInt();
                int min = local_response_data["testCase"]["rangeCaseList"][i]["min"].asInt();

                if (strstr(common_name.c_str(), "adc") != NULL) {
                    if (Board->Adc::adc_map.find(common_name) != Board->Adc::adc_map.end()) {
                        int adc_value = Board->get_adc_value(Board->Adc::adc_map[common_name].c_str());
                        local_response_data["testCase"]["rangeCaseList"][i]["testValue"] = adc_value;
                        if (adc_value >= min && adc_value <= max) {
                            local_response_data["testCase"]["rangeCaseList"][i]["testResult"] = "OK";
                        } else {
                            local_response_data["testCase"]["rangeCaseList"][i]["testResult"] = "NG";
                            local_response_data["testCase"]["testResult"] = "NG";
                            local_response["result"] = "false";
                        }
                    } else {
                        local_response_data["testCase"]["rangeCaseList"][i]["testValue"] = "NULL";
                        local_response_data["testCase"]["rangeCaseList"][i]["testResult"] = "NG";
                        local_response_data["testCase"]["testResult"] = "NG";
                        local_response["result"] = "false";
                    }
                }
            }

            local_response["cmdType"] = 1;
            local_response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
            local_response["data"] = local_response_data;
            protocol_.sendResponse(local_response);

            log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "Common test thread exit.");
        });
    interrupt_flags_queue_task.push(common_task_stop_flag);
}

// TODO : these handlers need to be combined
void TaskHandler::handleHandshake(const Task& task) {
    Json::Value response;
    response["cmdType"] = 2;
    response["desc"] = "okay";
    response["result"] = true;
    response["subCommand"] = CMD_HANDSHAKE;
    Json::Value data;
    data["model"] = "CMD3588_TESTAPP";
    data["version"] = "CMD3588_TESTAPP_V20250923_1";
    response["data"] = data;
    protocol_.sendResponse(response, task.cmdIndex);
}

void TaskHandler::handleBeginTest(const Task& task) {
    Json::Value response;
    response["cmdType"] = 2;
    response["result"] = true;
    response["subCommand"] = CMD_BEGIN_TEST;
    response["desc"] = "xxx";
    Json::Value data;
    response["data"] = data;
    protocol_.sendResponse(response, task.cmdIndex);
}

void TaskHandler::handleOverTest(const Task& task) {
    Json::Value response;
    response["cmdType"] = 2;
    response["result"] = true;
    response["subCommand"] = CMD_OVER_TEST;
    response["desc"] = "xxx";
    Json::Value data;
    response["data"] = data;
    protocol_.sendResponse(response, task.cmdIndex);
}

void TaskHandler::handleSignalToBeMeasured(const Task& task) {
    Json::Value response;
    response["cmdType"] = 2;
    response["result"] = true;
    response["subCommand"] = CMD_SIGNAL_TOBEMEASURED;
    response["desc"] = "xxx";
    Json::Value data;
    response["data"] = data;
    protocol_.sendResponse(response, task.cmdIndex);
}

void TaskHandler::handleSetSysTime(const Task& task) {
    Json::Value response;
    response["cmdType"] = 2;
    response["result"] = true;
    response["subCommand"] = CMD_SET_SYS_TIME;
    response["desc"] = "xxx";
    Json::Value data;
    response["data"] = data;
    protocol_.sendResponse(response, task.cmdIndex);    
}

void TaskHandler::handleSignalExec(const Task& task) {
    Json::Value response;
    response["cmdType"] = 2;
    response["result"] = true;
    response["subCommand"] = CMD_SIGNAL_EXEC;
    response["desc"] = "xxx";
    Json::Value data;
    response["data"] = data;
    protocol_.sendResponse(response, task.cmdIndex);
}

void TaskHandler::handleSignalToBeMeasuredCombine(const Task& task) {
    Json::Value response;
    response["cmdType"] = 2;
    response["result"] = true;
    response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_COMBINE;
    response["desc"] = "xxx";
    Json::Value data;
    response["data"] = data;
    protocol_.sendResponse(response, task.cmdIndex);
}

Json::Value TaskHandler::buildResponse(const Task& task, const TestResult& result, const std::string& testName) {
    Json::Value response;
    response["subCommand"] = result.responseCommand;
    response["type"] = testName;
    response["cmdIndex"] = task.cmdIndex;
    response["success"] = result.success;
    response["errorCode"] = result.errorCode;
    response["errorMsg"] = result.errorMsg;
    response["data"] = result.data;
    return response;
}

TestItem TaskHandler::stringToTestItem(const std::string& str) {
    if (str == "microphone") return MICROPHONE;
    if (str == "serial") return SERIAL;
    if (str == "gpio") return GPIO;
    if (str == "net") return NET;
    if (str == "rtc") return RTC;
    if (str == "storage") return STORAGE;
    if (str == "ddr") return DDR;
    if (str == "emmc") return EMMC;
    if (str == "wifi") return WIFI;
    if (str == "bluetooth") return BLUETOOTH;
    if (str == "base") return BASE;
    if (str == "switchs") return SWITCHS;
    if (str == "camera") return CAMERA;
    if (str == "typec") return TYPEC;
    if (str == "common") return COMMON;

    // TODO: 注意使用模糊匹配时的顺序，防止误判，防止均包含关键字符串
    if (strstr(str.c_str(), "facility") != NULL && strstr(str.c_str(), "3.0")) return FACILITYUSB3_0;  // 包含facilityUsb的字符串都认为是工装USB测试
    if (strstr(str.c_str(), "facility") != NULL && strstr(str.c_str(), "2.0")) return FACILITYUSB2_0;  // 包含facilityUsb的字符串都认为是工装USB测试
    if (strstr(str.c_str(), "usb_tf") != NULL) return TF;                                              // 包含tf的字符串都认为是TF卡测试
    if (strstr(str.c_str(), "usb_pcie") != NULL) return USB_PCIE;                                      // 包含usb_pcie的字符串都认为是USB_PCIE测试
    if (strstr(str.c_str(), "usb") != NULL) return USB;                                                // 包含usb的字符串都认为是U盘测试，放在usb_tf和usb_pcie后面 !!!!!!!!!!!!!
    if (strstr(str.c_str(), "232") != NULL) return SERIAL_232;                                         // 包含232的字符串都认为是232测试
    if (strstr(str.c_str(), "485") != NULL) return SERIAL_485;                                         // 包含485的字符串都认为是485测试
    if (strstr(str.c_str(), "can") != NULL) return CAN;                                                // 包含can的字符串都认为是can测试
    return NONE;
}
