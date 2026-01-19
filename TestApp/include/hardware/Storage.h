#ifndef __STORAGE_H__
#define __STORAGE_H__

#include <regex>
#include <memory>
#include <string>
#include <fstream>
#include <iostream>
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>
#include <fcntl.h>
#include <cmath>
#include <sys/sysinfo.h> 
#include <libudev.h>  
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libusb-1.0/libusb.h>
#include <sys/statvfs.h>
#include <mntent.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <map>
#include <vector>
#include <sstream> 

#include "util/Log.h"

class Storage {
private:
    const char* STORAGE_TAG = "STORAGE";

public:
    const char* MMC_DEVICE_SIZE_PATH;
    const char* PCIE_DEVICE_SIZE_PATH;
    
    Storage(const char* mmcDeviceSizePath = "/sys/block/mmcblk0/size",
            const char* pcieDeviceSizePath = "/sys/block/nvme0n1/size");  
    /* 
     * @brief 获取ddr大小
     */
    virtual float getDdrSize();

    
    /* 
     * @brief 获取emmc大小
     */
    virtual float getEmmcSize();

    /* 
     * @brief 获取emmc大小
     */
    virtual float getPcieSize();

    /*
     * @brief 判断是否为usb设备
     */
    virtual bool isUsbDevice(struct udev_device *dev);

    /*
     * @brief 获取u盘大小
     */
    virtual float getUdiskSize();

    // TODO: 暂未做成接口，需要优化。
    // 获取u盘容量大小

    std::vector<float> usbDiskSizeList;
    int usbDiskCount = 0;
    bool g_size_found = false;

    struct FacilityusbInfo {
        uint16_t vid;
        uint16_t pid;
        std::string usbType;
    };
    std::vector<FacilityusbInfo> facilityUsbInfoList3_0;
    int facilityUsbCount3_0 = 0;
    std::vector<FacilityusbInfo> facilityUsbInfoList2_0;
    int facilityUsbCount2_0 = 0;

    const char* get_device_class_name(uint8_t class_code);
    int is_mass_storage_device_by_interface(libusb_device* dev);
    char* find_usb_device_node(uint16_t vid, uint16_t pid);
    char* find_usb_mount_path(uint16_t vid, uint16_t pid);
    int get_device_capacity_by_node(const char* device_node, unsigned long long* total_size);
    int get_mounted_capacity(const char* mount_path, unsigned long long* total_size, 
                        unsigned long long* free_size, unsigned long long* used_size);
    void display_capacity_info(const char* device_node, const char* mount_path);
    void get_detailed_device_type(libusb_device* dev, uint8_t class_code, 
                             uint8_t subclass, uint8_t protocol, char* type_desc);
    void search_directory(const char *path);
    void scan_usb_with_libusb();



    struct lsusbFacilityusbInfo {
        uint16_t vid;
        uint16_t pid;
    };
    int lsusbFacilityUsbCount = 0;
    std::vector<lsusbFacilityusbInfo> lsusbFacilityUsbInfoList;    // 后来测试过程中发现usb小板3.0  2.0  pid vid 不同，且不同小板之间也不同。不需要区分   
    virtual bool lsusbGetVidPidInfo();   


private:

};

#endif