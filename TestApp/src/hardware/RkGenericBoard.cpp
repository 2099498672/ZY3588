#include "hardware/RkGenericBoard.h"

RkGenericBoard::RkGenericBoard() {

}

RkGenericBoard::RkGenericBoard(
                                const char* mmcDeviceSizePath, const char* pcieDeviceSizePath,                          /* Storage */
                                const char* tfCardDeviceSizePath,                                                         /* Tf */
                                const char* rledPath, LED_CTRL rled_status, const char* gledPath, LED_CTRL gled_status, /* Led */
                                const char* hwidPath,                                                                   /* BaseInfo */
                                const char* audioEventName, const char* audioCardName,                                   /* Audio */

                                const char* fanCtrlPath, FAN_CTRL initialStatus, 
                                const char* interface, int scanCount,
                                const std::string& rtcDevicePath)
    : 
      Storage(mmcDeviceSizePath, pcieDeviceSizePath),
      Tf(tfCardDeviceSizePath),
      // Key(keyEventPath, keyTimeOut),
      Led(rledPath, rled_status, gledPath, gled_status),
      BaseInfo(hwidPath),
      Audio(audioEventName, audioCardName),

      Fan(fanCtrlPath, initialStatus),
      Wifi(interface, scanCount),
      Rtc(rtcDevicePath) {

    log_thread_safe(LOG_LEVEL_INFO, "RkGenericBoard", "mmcDeviceSizePath: %s", mmcDeviceSizePath);
    log_thread_safe(LOG_LEVEL_INFO, "RkGenericBoard", "tfCardDeviceSizePath: %s", tfCardDeviceSizePath);
    log_thread_safe(LOG_LEVEL_INFO, "RkGenericBoard", "rledPath: %s, rled_status: %d", rledPath, rled_status);
    log_thread_safe(LOG_LEVEL_INFO, "RkGenericBoard", "gledPath: %s, gled_status: %d", gledPath, gled_status);
    log_thread_safe(LOG_LEVEL_INFO, "RkGenericBoard", "hwidPath: %s", hwidPath); 
    log_thread_safe(LOG_LEVEL_INFO, "RkGenericBoard", "audioEventName: %s, audioCardName:%s", audioEventName, audioCardName);
    log_thread_safe(LOG_LEVEL_INFO, "RkGenericBoard", "fanCtrlPath: %s, initialStatus: %d", fanCtrlPath, initialStatus);
    log_thread_safe(LOG_LEVEL_INFO, "RkGenericBoard", "wifi interface: %s, scanCount: %d", interface, scanCount);
    log_thread_safe(LOG_LEVEL_INFO, "RkGenericBoard", "rtcDevicePath: %s", rtcDevicePath.c_str());
}

std::string RkGenericBoard::get_board_name() {
    return board_name;
}