#include "project/ZY3588/ZY3588.h"

ZY3588::ZY3588() {
}

ZY3588::ZY3588( 
                const char* mmcDeviceSizePath, const char* pcieDeviceSizePath,      /* Storage */
                const char* tfCardDeviceSizePath,   /* Tf */
                // const char* keyEventPath, int keyTimeOut,   /* Key */
                const char* rledPath, LED_CTRL rled_status, const char* gledPath, LED_CTRL gled_status, /* Led */
                const char* hwidPath, /* BaseInfo */
                const char* audioEventName, const char* audioCardName,  /* audio */

                const char* fanCtrlPath, FAN_CTRL initialStatus,
                const char* interface, int scanCount,
                const std::string& rtcDevicePath)
    :   RkGenericBoard( 
                        mmcDeviceSizePath, pcieDeviceSizePath,  /* Storage */
                        tfCardDeviceSizePath,   /* Tf */
                        // keyEventPath, keyTimeOut,   /* Key */
                        rledPath, rled_status, gledPath, gled_status, /* Led */
                        hwidPath, /* BaseInfo */
                        audioEventName, audioCardName,  /* audio */

                        fanCtrlPath, initialStatus, 
                        interface, scanCount,
                        rtcDevicePath) {
    std::string board_name = "RkGenericBoard";

    Key::keyEventNamesToPath["adc-keys"] = "null";
    Key::keyEventNamesToPath["febd0030.pwm"] = "null";     
    Key::keyMap[114] = "VOL-";   
    Key::keyMap[115] = "VOL+";
    Key::keyMap[139] = "MENU";
    Key::keyMap[158] = "ESC";
    Key::keyMap[28] = "OK";

    Camera::cameraidToInfo["cam1"] = { "cam1", "/dev/video73", "/root/cam1_test.png" };   // 丝印 cam0
    Camera::cameraidToInfo["cam2"] = { "cam2", "/dev/video55", "/root/cam2_test.png" };   // 丝印 cam2
    Camera::cameraidToInfo["cam3"] = { "cam3", "/dev/video64", "/root/cam3_test.png" };   // 丝印 cam3
    Camera::cameraidToInfo["cam4"] = { "cam4", "/dev/video91", "/root/cam4_test.png" };   // 丝印 cam1
    Camera::cameraidToInfo["cam5"] = { "cam5", "/dev/video82", "/root/cam5_test.png" };   // 丝印 cam1

    Adc::adc_map["adc4"] = "/sys/bus/iio/devices/iio:device0/in_voltage4_raw";
    Adc::adc_map["adc5"] = "/sys/bus/iio/devices/iio:device0/in_voltage5_raw";
    Adc::adc_map["adc6"] = "/sys/bus/iio/devices/iio:device0/in_voltage6_raw";
    Adc::adc_map["adc7"] = "/sys/bus/iio/devices/iio:device0/in_voltage7_raw";

        
    // LogInfo("ZY3588", "ZY3588 with Fan initialized");
    // LogInfo("ZY3588", "mmcDeviceSizePath: %s", mmcDeviceSizePath);
    // LogInfo("ZY3588", "pcieDeviceSizePath: %s", pcieDeviceSizePath);
    // LogInfo("ZY3588", "tfCardDeviceSizePath: %s", tfCardDeviceSizePath);
    // LogInfo("ZY3588", "rledPath: %s, rled_status: %d", rledPath, rled_status);
    // LogInfo("ZY3588", "gledPath: %s, gled_status: %d", gledPath, gled_status);
    // LogInfo("ZY3588", "hwidPath: %s", hwidPath);
    // LogInfo("ZY3588", "audioEventName: %s", audioEventName);
    // LogInfo("ZY3588", "fanCtrlPath: %s", fanCtrlPath);
    // LogInfo("ZY3588", "interface: %s, scanCount: %d", interface, scanCount);
    // LogInfo("ZY3588", "rtcDevicePath: %s", rtcDevicePath.c_str());

    log_thread_safe(LOG_LEVEL_INFO, "ZY3588", "ZY3588 with Fan initialized");
    log_thread_safe(LOG_LEVEL_INFO, "ZY3588", "mmcDeviceSizePath: %s", mmcDeviceSizePath);
    log_thread_safe(LOG_LEVEL_INFO, "ZY3588", "pcieDeviceSizePath: %s", pcieDeviceSizePath);
    log_thread_safe(LOG_LEVEL_INFO, "ZY3588", "tfCardDeviceSizePath: %s", tfCardDeviceSizePath);
    log_thread_safe(LOG_LEVEL_INFO, "ZY3588", "rledPath: %s, rled_status: %d", rledPath, rled_status);
    log_thread_safe(LOG_LEVEL_INFO, "ZY3588", "gledPath: %s, gled_status: %d", gledPath, gled_status);
    log_thread_safe(LOG_LEVEL_INFO, "ZY3588", "hwidPath: %s", hwidPath);
    log_thread_safe(LOG_LEVEL_INFO, "ZY3588", "audioEventName: %s", audioEventName);
    log_thread_safe(LOG_LEVEL_INFO, "ZY3588", "fanCtrlPath: %s", fanCtrlPath);
    log_thread_safe(LOG_LEVEL_INFO, "ZY3588", "interface: %s, scanCount: %d", interface, scanCount);
    log_thread_safe(LOG_LEVEL_INFO, "ZY3588", "rtcDevicePath: %s", rtcDevicePath.c_str());
}

// bool ZY3588::get_firmware_version(std::string& firmware_version) {
//     firmware_version = "ZY3588_RK3588_Ubuntu20.04_20241015.101112";
//     return true;
// }

bool ZY3588::get_app_version(std::string& app_version) {
    app_version = "CMBOARD_TESTAPP_ZY3588_V20251216_103800";
    return true;
}

std::string ZY3588::get_board_name() {
    return board_name;
}