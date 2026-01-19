#include "project/BoardFactory.h"
#include "project/ZY3588/ZY3588.h"
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

#include "util/Log.h"

const char *BOARD_FACTORY_TAG = "BoardFactory";
  
extern std::queue<std::shared_ptr<interrupt_flag>> interrupt_flags_queue_main;
extern thread_pool work_thread_main;


std::shared_ptr<RkGenericBoard> BoardFactory::create_board_v1(const std::string& board_name) {
    BOARD_NAME board_name_enum = BoardFactory::string_to_enum(board_name);
    switch(board_name_enum) {

        case BOARD_NAME::BOARD_ZY3588: {
            log_thread_safe(LOG_LEVEL_INFO, BOARD_FACTORY_TAG, "Creating BOARD_ZY3588 board instance");
            uart = new Uart("/dev/ttyS0");
            if (!uart->open()) {
                log_thread_safe(LOG_LEVEL_ERROR, BOARD_FACTORY_TAG, "cannot open uart, exiting program");
                delete uart;
                return nullptr;
            }

            if (!uart->configure(115200, 8, 'N', 1)) {
                log_thread_safe(LOG_LEVEL_ERROR, BOARD_FACTORY_TAG, "cannot configure uart, exiting program");
                uart->close();
                delete uart;
                return nullptr;
            }
            std::shared_ptr<RkGenericBoard> board = std::make_shared<ZY3588>(
                "/sys/class/block/mmcblk0/size", "/sys/class/block/nvme0n1/size",                                       /* Storage  : emmc path, pcie path*/
                "/sys/class/block/mmcblk1/size",                                                                        /* Tf       : tf card path */
                "/sys/class/leds/led_red/brightness", LED_OFF, "/sys/class/leds/led_green/brightness", LED_OFF,         /* Led      : rled path, rled status, gled path, gled status */
                "/proc/device-tree/hw-board/hw-version",                                                                /* BaseInfo : hwid path */
                "rockchip-es8389 Headset", "plughw:rockchipes8389" ,                                                    /* audio    : audio event name */
                "/sys/class/misc/fan-ctl/pwr_en", FAN_OFF,                                                              /* Fan      : fan ctrl path, initial status */
                "wlan0", 5,                                                                                             /* Wifi     : interface, scan count */
                "/dev/rtc0"                                                                                             /* Rtc      : rtc device path */
            );

            std::shared_ptr<interrupt_flag> led_shink_flag = 
                work_thread_main.submit_interruptible([board_ptr = board](interrupt_flag& flag) {                                                                                                              // board_ptr 指向的是堆区对象，后面std::move(board);了。 
                                                                                                                                                                                                                // 外面函数千万不要reset这个指针，让它自然析构，设计缺陷请后面再优化   /(ㄒoㄒ)/~~
                                                                                                                                                                                   // 目前情况是大部分线程持有的board指针都是这个board_ptr指针，而线程池只有在程序退出时才析构，所以不会有野指针问题。
                    while (!flag.is_stop_requested()) {
                        board_ptr->setRledStatus(LED_ON);
                        sleep(1);
                        board_ptr->setRledStatus(LED_OFF);
                        sleep(1);
                    }
                    log_thread_safe(LOG_LEVEL_INFO, BOARD_FACTORY_TAG, "led shink task exit, thread end");
                });
            interrupt_flags_queue_main.push(led_shink_flag);

            std::shared_ptr<interrupt_flag> audio_test_flag = 
                work_thread_main.submit_interruptible([board_ptr = board](interrupt_flag& flag) {
                    if (board_ptr->findEventDevice() == false) {
                        log_thread_safe(LOG_LEVEL_ERROR, BOARD_FACTORY_TAG, "cannot find audio input device, audio test function unavailable");
                        return;
                    }
                    while (!flag.is_stop_requested()) {
                        board_ptr->selectAudioIn();                                                                                                                                                              // 这里一定不能堵塞哟 ！！！！！！！！！ ~(≧▽≦)~
                    }
                    log_thread_safe(LOG_LEVEL_INFO, BOARD_FACTORY_TAG, "audio test task exit, thread end");
                });
            interrupt_flags_queue_main.push(audio_test_flag);

            return board;
        }        

        default:
            log_thread_safe(LOG_LEVEL_ERROR, BOARD_FACTORY_TAG, "Unknown board name string: %s", board_name.c_str());
            return nullptr;
    }
}

BOARD_NAME BoardFactory::string_to_enum(const std::string& board_name_str) {
    if (board_name_str == "ZY3588") {
        return BOARD_NAME::BOARD_ZY3588;
    } else {
        log_thread_safe(LOG_LEVEL_ERROR, BOARD_FACTORY_TAG, "Unknown board name string: %s", board_name_str.c_str());
        return BOARD_NAME::BOARD_UNKNOWN;
    }
}

BoardFactory::~BoardFactory() {
    if (uart) {
        uart->close();
        delete uart;
        uart = nullptr;
    }
}