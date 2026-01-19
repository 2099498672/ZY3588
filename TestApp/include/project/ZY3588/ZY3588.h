#ifndef __ZY3588_H__
#define __ZY3588_H__

#include "hardware/RkGenericBoard.h"
#include "util/Log.h"

class ZY3588 : public RkGenericBoard {
public:
    ZY3588();
    ZY3588( 
            const char* mmcDeviceSizePath, const char* pcieDeviceSizePath,     /* Storage */
            const char* tfCardDeviceSizePath,   /* Tf */
            // const char* keyEventPath, int keyTimeOut,   /* Key */
            const char* rledPath, LED_CTRL rled_status, const char* gledPath, LED_CTRL gled_status, /* Led */
            const char* hwidPath, /* BaseInfo */
            const char* audioEventName,  const char* audioCardName,  /* audio *//* audio */

            const char* fanCtrlPath, FAN_CTRL initialStatus, 
            const char* interface, int scanCount,
            const std::string& rtcDevicePath);


    // bool get_firmware_version(std::string& firmware_version) override;
    bool get_app_version(std::string& app_version) override;

    std::string board_name = "ZY3588";
    std::string get_board_name() override;
};

#endif