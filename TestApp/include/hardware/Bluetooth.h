#ifndef __BLUETOOTH_H__
#define __BLUETOOTH_H__

#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <errno.h>  // 新增：用于错误码处理
#include "util/Log.h"

class Bluetooth {
public:
    Bluetooth();
    ~Bluetooth();
    const char* BLUETOOTH_TAG = "BLUETOOTH";

    // 蓝牙设备信息结构体（已包含rssi字段）
    typedef struct {
        char macAddress[18];   // MAC地址 (17字符+1空字符)
        char name[248];        // 设备名称
        int rssi;              // 信号强度（新增功能：需填充）
    } BluetoothDevice;

    // 扫描结果结构体
    typedef struct {
        int count;                // 发现的设备数量
        BluetoothDevice *devices; // 设备数组指针
    } BluetoothScanResult;

    BluetoothScanResult bluetoothScanResult = {.count = -1, .devices = NULL};

    // 扫描蓝牙设备（需修改实现以获取RSSI）
    virtual bool scanBluetoothDevices(int duration = 8);
    // 打印扫描结果（需修改以显示RSSI）
    virtual void printScanResult();
    // 释放资源（无需修改）
    virtual void freeScanResult();
};

#endif