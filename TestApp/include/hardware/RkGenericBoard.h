#ifndef __RK_GENERIC_BOARD_H__
#define __RK_GENERIC_BOARD_H__

#include "common/Constants.h"

#include "hardware/Storage.h"
#include "hardware/Tf.h"
#include "hardware/TypeC.h"
#include "hardware/Adc.h"
#include "hardware/Gpio.h"
#include "hardware/Fan.h"
#include "hardware/Key.h"
#include "hardware/Wifi.h"  
#include "hardware/Bluetooth.h"
#include "hardware/Serial.h"
#include "hardware/Net.h"
#include "hardware/Rtc.h"
#include "hardware/Camera.h"
#include "hardware/Led.h"
#include "hardware/Audio.h"
#include "hardware/BaseInfo.h"
#include "hardware/VendorStorage.h"
#include "hardware/GetDpLanes.h"

#include "util/Log.h"

// 待测项目路径不同可通过构造函数传入， 差异较大请重写方法
class RkGenericBoard :  public Storage, public Tf, public Key, public Gpio, public Fan, public Wifi, public Bluetooth, 
                        public Serial, public Net, public Rtc, public Camera, public TypeC, public Adc, public Led,
                        public Audio, public VendorStorage, public GetDpLanes, public BaseInfo {
public:
    RkGenericBoard();
    RkGenericBoard( 
                    const char* mmcDeviceSizePath, const char* pcieDeviceSizePath,                          /* Storage */
                    const char* tfCardDeviceSizePath,                                                       /* Tf */
                    const char* rledPath, LED_CTRL rled_status, const char* gledPath, LED_CTRL gled_status, /* Led */
                    const char* hwidPath,                                                                   /* BaseInfo */
                    const char* audioEventName, const char* audioCardName,                                  /* Audio */
                
                    const char* fanCtrlPath, FAN_CTRL initialStatus, 
                    const char* interface, int scanCount,
                    const std::string& rtcDevicePath);

    std::string board_name = "RkGenericBoard";
    virtual std::string get_board_name();
    static BOARD_NAME string_to_enum(const std::string board_name_str);
};

#endif