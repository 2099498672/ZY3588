#include "hardware/Storage.h"

Storage::Storage(const char* mmcDeviceSizePath, const char* pcieDeviceSizePath) 
    : MMC_DEVICE_SIZE_PATH(mmcDeviceSizePath), PCIE_DEVICE_SIZE_PATH(pcieDeviceSizePath) ,
    usbDiskSizeList(10), facilityUsbInfoList2_0(10), facilityUsbInfoList3_0(10), lsusbFacilityUsbInfoList(30) {

}

float Storage::getDdrSize() {
    struct sysinfo info;
    if (sysinfo(&info) != 0) {
        log_thread_safe(LOG_LEVEL_ERROR, STORAGE_TAG, "sysinfo failed!");
        return -1;
    }

    unsigned long long totalKb = info.totalram * info.mem_unit / 1024;              // sysinfo returns total memory in bytes, convert to kB
    // unsigned long long totalMb = totalKb / 1024;
    float totalGb = static_cast<float>(totalKb) / (1024 * 1024);                    // convert to GB and keep two decimal places
    float roundedGb = round(totalGb * 100.0f) / 100.0f;
    log_thread_safe(LOG_LEVEL_INFO, STORAGE_TAG, "ddr total size: %llu KB (%.2f GB)", totalKb, roundedGb);
    return roundedGb;
}

float Storage::getEmmcSize() {
    FILE* fp = fopen(MMC_DEVICE_SIZE_PATH, "r");
    if (!fp) {
        log_thread_safe(LOG_LEVEL_ERROR, STORAGE_TAG, "can not open : %s", MMC_DEVICE_SIZE_PATH);
        return -1;
    }

    unsigned long long sectors;
    if (fscanf(fp, "%llu", &sectors) != 1) {
        log_thread_safe(LOG_LEVEL_ERROR, STORAGE_TAG, "read emmc sectors failed. ");
        fclose(fp);
        return -1;
    }
    fclose(fp);

    unsigned long long totalBytes = sectors * 512;                                      // calculate total bytes and KB. every sector is 512 bytes
    unsigned long long totalKb = totalBytes / 1024;                                     // convert to KB
    // unsigned long long totalMb = totalBytes / (1024 * 1024);
    
    float totalGb = static_cast<float>(totalBytes) / (1024.0f * 1024.0f * 1024.0f);      // convert to GB and keep two decimal places
    float roundedGb = round(totalGb * 100.0f) / 100.0f;
    
    log_thread_safe(LOG_LEVEL_INFO, STORAGE_TAG, "emmc total size: %llu KB (%.2f GB)", totalKb, roundedGb);
    return roundedGb;
} 

float Storage::getPcieSize() {
    FILE* fp = fopen(PCIE_DEVICE_SIZE_PATH, "r");
    if (!fp) {
        log_thread_safe(LOG_LEVEL_ERROR, STORAGE_TAG, "can not open : %s", PCIE_DEVICE_SIZE_PATH);
        return -1;
    }

    unsigned long long sectors;
    if (fscanf(fp, "%llu", &sectors) != 1) {
        log_thread_safe(LOG_LEVEL_ERROR, STORAGE_TAG, "read pcie sectors failed. ");
        fclose(fp);
        return -1;
    }
    fclose(fp);

    unsigned long long totalBytes = sectors * 512;    // convert to bytes
    unsigned long long totalKb = totalBytes / 1024;   // convert to KB
    // unsigned long long totalMb = totalBytes / (1024 * 1024);
    
    float totalGb = static_cast<float>(totalBytes) / (1024.0f * 1024.0f * 1024.0f);      // convert to GB and keep two decimal places
    float roundedGb = round(totalGb * 100.0f) / 100.0f;
    
    log_thread_safe(LOG_LEVEL_INFO, STORAGE_TAG, "pcie total size: %llu KB (%.2f GB)", totalKb, roundedGb);
    return roundedGb;
}

bool Storage::isUsbDevice(struct udev_device *dev) {
    struct udev_device *current = dev;
    while (current) {
        const char *subsystem = udev_device_get_subsystem(current);
        if (subsystem && strcmp(subsystem, "usb") == 0) {
            return true;
        }
        current = udev_device_get_parent(current);
    }
    return false; 
}

float Storage::getUdiskSize() {
    struct udev *udev = udev_new();
    if (!udev) {
        log_thread_safe(LOG_LEVEL_ERROR, STORAGE_TAG, "can not create udev");
        return -1;
    }

    struct udev_enumerate *enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "block");
    udev_enumerate_scan_devices(enumerate);
 
    struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry *entry;
 
    bool found = false;
    double gb = -1.0;
    udev_list_entry_foreach(entry, devices) {
        const char *path = udev_list_entry_get_name(entry);
        LogDebug(STORAGE_TAG, "checking device at path: %s", path);
        struct udev_device *dev = udev_device_new_from_syspath(udev, path);
        if (!dev) {
            LogError(STORAGE_TAG, "can not get udev device from path: %s", path);
            continue;
        }
 
        const char *devtype = udev_device_get_devtype(dev);
        LogDebug(STORAGE_TAG, "devtype: %s", devtype ? devtype : "null");
 
        if (isUsbDevice(dev) && devtype && strcmp(devtype, "disk") == 0) {
            const char *devnode = udev_device_get_devnode(dev);
            LogDebug(STORAGE_TAG, "found usb device: %s", devnode ? devnode : "null"); 
            int fd = open(devnode, O_RDONLY);
            if (fd < 0) {
                LogError(STORAGE_TAG, "open device failed [%s]: %s", devnode, strerror(errno));
                udev_device_unref(dev);
                continue;
            }
 
            uint64_t size;
            if (ioctl(fd, BLKGETSIZE64, &size) == -1) {
                LogError(STORAGE_TAG, "get size ioctl BLKGETSIZE64 failed: %s", strerror(errno));
                close(fd);
                udev_device_unref(dev);
                continue;
            }
 
            gb = (double)size / (1024 * 1024 * 1024);
            LogDebug(STORAGE_TAG, "udisk total size: %.2f GB", gb);
            found = true;
 
            close(fd);
        }
        udev_device_unref(dev);
    }
 
    if (!found) {
        LogDebug(STORAGE_TAG, "no usb device found");
    }
 
    udev_enumerate_unref(enumerate);
    udev_unref(udev);
    return gb;
}



// 根据设备类代码返回设备类型描述
const char* Storage::get_device_class_name(uint8_t class_code) {
    switch (class_code) {
        case 0x00: return "接口描述符中定义";
        case 0x01: return "音频设备";
        case 0x02: return "通信设备";
        case 0x03: return "人机接口设备(HID)";
        case 0x05: return "物理设备";
        case 0x06: return "图像设备";
        case 0x07: return "打印机";
        case 0x08: return "大容量存储设备";
        case 0x09: return "集线器";
        case 0x0A: return "CDC数据设备";
        case 0x0B: return "智能卡";
        case 0x0D: return "安全设备";
        case 0x0E: return "视频设备";
        case 0x0F: return "医疗设备";
        case 0x10: return "音频/视频设备";
        case 0x11: return "账单设备";
        case 0xDC: return "诊断设备";
        case 0xE0: return "无线控制器";
        case 0xEF: return "混合设备";
        case 0xFE: return "特定应用设备";
        case 0xFF: return "厂商特定设备";
        default: return "未知设备";
    }
}

// 检查接口描述符判断是否为存储设备
int Storage::is_mass_storage_device_by_interface(libusb_device* dev) {
    struct libusb_config_descriptor* config;
    int r = libusb_get_config_descriptor(dev, 0, &config);
    if (r < 0) return 0;
    
    int is_storage = 0;
    for (int i = 0; i < config->bNumInterfaces; i++) {
        const struct libusb_interface* interface = &config->interface[i];
        for (int j = 0; j < interface->num_altsetting; j++) {
            const struct libusb_interface_descriptor* altsetting = &interface->altsetting[j];
            if (altsetting->bInterfaceClass == 0x08) { // 大容量存储设备类
                is_storage = 1;
                break;
            }
        }
        if (is_storage) break;
    }
    
    libusb_free_config_descriptor(config);
    return is_storage;
}

// 根据VID/PID查找U盘对应的设备节点
char* Storage::find_usb_device_node(uint16_t vid, uint16_t pid) {
    DIR* dir;
    struct dirent* entry;
    char* device_node = NULL;
    
    // 检查常见的块设备目录
    const char* block_dirs[] = {"/sys/block", "/sys/class/block", NULL};
    
    for (int i = 0; block_dirs[i] != NULL; i++) {
        dir = opendir(block_dirs[i]);
        if (!dir) continue;
        
        while ((entry = readdir(dir)) != NULL) {
            // 只关心sd、mmcblk等常见的可移动设备
            if (strncmp(entry->d_name, "sd", 2) == 0 || 
                strncmp(entry->d_name, "mmcblk", 6) == 0) {
                
                char device_path[256];
                char vendor_path[256];
                char product_path[256];
                FILE* vendor_file;
                FILE* product_file;
                char vid_str[10], pid_str[10];
                uint16_t found_vid, found_pid;
                
                // 构建路径查找VID/PID
                snprintf(device_path, sizeof(device_path), 
                        "%s/%s/device", block_dirs[i], entry->d_name);
                
                snprintf(vendor_path, sizeof(vendor_path), 
                        "%s/idVendor", device_path);
                snprintf(product_path, sizeof(product_path), 
                        "%s/idProduct", device_path);
                
                vendor_file = fopen(vendor_path, "r");
                if (vendor_file) {
                    if (fgets(vid_str, sizeof(vid_str), vendor_file)) {
                        found_vid = (uint16_t)strtoul(vid_str, NULL, 16);
                        
                        product_file = fopen(product_path, "r");
                        if (product_file) {
                            if (fgets(pid_str, sizeof(pid_str), product_file)) {
                                found_pid = (uint16_t)strtoul(pid_str, NULL, 16);
                                
                                if (found_vid == vid && found_pid == pid) {
                                    snprintf(device_path, sizeof(device_path), 
                                            "/dev/%s", entry->d_name);
                                    device_node = strdup(device_path);
                                    fclose(product_file);
                                    fclose(vendor_file);
                                    closedir(dir);
                                    return device_node;
                                }
                            }
                            fclose(product_file);
                        }
                    }
                    fclose(vendor_file);
                }
            }
        }
        closedir(dir);
    }
    
    return NULL;
}

// 根据VID/PID查找U盘在系统中的挂载路径
char* Storage::find_usb_mount_path(uint16_t vid, uint16_t pid) {
    FILE* mntfile = setmntent("/proc/mounts", "r");
    if (!mntfile) return NULL;
    
    struct mntent* mnt;
    char* mount_path = NULL;
    
    while ((mnt = getmntent(mntfile)) != NULL) {
        // 检查是否是块设备
        if (strncmp(mnt->mnt_fsname, "/dev/sd", 7) == 0 || 
            strncmp(mnt->mnt_fsname, "/dev/mmcblk", 11) == 0) {
            
            char device_info_path[256];
            char vid_pid[32];
            FILE* info_file;
            
            // 通过sysfs查找设备的VID/PID
            const char* dev_name = strrchr(mnt->mnt_fsname, '/') + 1;
            snprintf(device_info_path, sizeof(device_info_path), 
                    "/sys/block/%s/device/../idVendor", dev_name);
            
            info_file = fopen(device_info_path, "r");
            if (info_file) {
                if (fgets(vid_pid, sizeof(vid_pid), info_file)) {
                    uint16_t found_vid = (uint16_t)strtoul(vid_pid, NULL, 16);
                    
                    snprintf(device_info_path, sizeof(device_info_path), 
                            "/sys/block/%s/device/../idProduct", dev_name);
                    
                    FILE* pid_file = fopen(device_info_path, "r");
                    if (pid_file) {
                        if (fgets(vid_pid, sizeof(vid_pid), pid_file)) {
                            uint16_t found_pid = (uint16_t)strtoul(vid_pid, NULL, 16);
                            
                            if (found_vid == vid && found_pid == pid) {
                                mount_path = strdup(mnt->mnt_dir);
                                fclose(pid_file);
                                fclose(info_file);
                                break;
                            }
                        }
                        fclose(pid_file);
                    }
                }
                fclose(info_file);
            }
        }
    }
    
    endmntent(mntfile);
    return mount_path;
}

// 通过设备节点获取存储设备容量（适用于未挂载设备）
int Storage::get_device_capacity_by_node(const char* device_node, unsigned long long* total_size) {
    int fd = open(device_node, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    
    // 使用ioctl获取设备大小
    if (ioctl(fd, BLKGETSIZE64, total_size) < 0) {
        close(fd);
        return -1;
    }
    
    close(fd);
    return 0;
}

// 获取存储设备容量信息（适用于已挂载设备）
int Storage::get_mounted_capacity(const char* mount_path, unsigned long long* total_size, 
                        unsigned long long* free_size, unsigned long long* used_size) {
    struct statvfs stat;
    
    if (statvfs(mount_path, &stat) != 0) {
        return -1;
    }
    
    *total_size = (unsigned long long)stat.f_blocks * stat.f_frsize;
    *free_size = (unsigned long long)stat.f_bfree * stat.f_frsize;
    *used_size = *total_size - *free_size;
    
    return 0;
}

// 显示容量信息
void Storage::display_capacity_info(const char* device_node, const char* mount_path) {
    unsigned long long total_size = 0, free_size = 0, used_size = 0;
    
    if (mount_path) {
        // 设备已挂载，使用statvfs获取详细信息
        if (get_mounted_capacity(mount_path, &total_size, &free_size, &used_size) == 0) {
            printf("  挂载路径: %s\n", mount_path);
            printf("  存储容量信息:\n");
            printf("    总容量: %.2f GB\n", total_size / (1024.0 * 1024.0 * 1024.0));
            printf("    已使用: %.2f GB\n", used_size / (1024.0 * 1024.0 * 1024.0));
            printf("    可用空间: %.2f GB\n", free_size / (1024.0 * 1024.0 * 1024.0));
            printf("    使用率: %.1f%%\n", (used_size * 100.0) / total_size);
        }
    } else if (device_node) {
        // 设备未挂载，通过设备节点获取总容量
        if (get_device_capacity_by_node(device_node, &total_size) == 0) {
            printf("  设备节点: %s\n", device_node);
            printf("  存储容量信息:\n");
            printf("    总容量: %.2f GB (设备未挂载)\n", total_size / (1024.0 * 1024.0 * 1024.0));
            printf("    使用情况: 需要挂载后才能查看详细使用信息\n");
        } else {
            printf("  无法读取设备容量信息\n");
        }
    } else {
        printf("  未找到对应的设备节点\n");
    }
}

// 获取详细的设备类型描述
void Storage::get_detailed_device_type(libusb_device* dev, uint8_t class_code, 
                             uint8_t subclass, uint8_t protocol, char* type_desc) {
    // 首先检查接口描述符来判断设备类型
    if (is_mass_storage_device_by_interface(dev)) {
        struct libusb_config_descriptor* config;
        if (libusb_get_config_descriptor(dev, 0, &config) == 0) {
            for (int i = 0; i < config->bNumInterfaces; i++) {
                const struct libusb_interface* interface = &config->interface[i];
                for (int j = 0; j < interface->num_altsetting; j++) {
                    const struct libusb_interface_descriptor* altsetting = &interface->altsetting[j];
                    if (altsetting->bInterfaceClass == 0x08) {
                        switch (altsetting->bInterfaceSubClass) {
                            case 0x02: 
                                if (altsetting->bInterfaceProtocol == 0x50) {
                                    sprintf(type_desc, "CD/DVD光驱");
                                } else {
                                    sprintf(type_desc, "ATAPI设备");
                                }
                                break;
                            case 0x06: sprintf(type_desc, "可启动存储设备"); break;
                            default: sprintf(type_desc, "USB存储设备(U盘/移动硬盘)"); break;
                        }
                        libusb_free_config_descriptor(config);
                        return;
                    }
                }
            }
            libusb_free_config_descriptor(config);
        }
        sprintf(type_desc, "USB存储设备");
    }
    else if (class_code == 0x03) { // HID设备
        if (subclass == 0x01) {
            if (protocol == 0x01) sprintf(type_desc, "键盘");
            else if (protocol == 0x02) sprintf(type_desc, "鼠标");
            else sprintf(type_desc, "HID设备");
        } else {
            sprintf(type_desc, "HID设备");
        }
    }
    else if (class_code == 0x02) {
        sprintf(type_desc, "网络设备");
    }
    else if (class_code == 0x09) {
        sprintf(type_desc, "USB集线器");
    }
    else if (class_code == 0x0E) {
        sprintf(type_desc, "视频设备");
    }
    else if (class_code == 0x01) {
        sprintf(type_desc, "音频设备");
    }
    else {
        sprintf(type_desc, "%s", get_device_class_name(class_code));
    }
}
/*=============================================================================*/
void Storage::search_directory(const char *path) {
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    char full_path[PATH_MAX];
  
    // 如果已经找到size文件，直接返回
    if (g_size_found) {
        return;
    }
  
    if (!(dir = opendir(path))) {
        perror("opendir");
        return;
    }
  
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
  
        snprintf(full_path, PATH_MAX, "%s/%s", path, entry->d_name);
  
        if (lstat(full_path, &statbuf) < 0) {
            perror("lstat");
            continue;
        }
  
        if (S_ISDIR(statbuf.st_mode)) {
            search_directory(full_path);
            // 检查子目录搜索后是否找到了size文件
            if (g_size_found) {
                closedir(dir);
                return;
            }
        } else if (S_ISREG(statbuf.st_mode) && strcmp(entry->d_name, "size") == 0) {
            // printf("Found: %s\n", full_path);
            int fd = open(full_path, O_RDONLY);
            if (fd < 0) {
                perror("open size file");
                closedir(dir);
                return;
            }

            char size_str[32];
            if (read(fd, size_str, sizeof(size_str)-1) > 0) {
                unsigned long long size = strtoull(size_str, NULL, 10);
                // printf("容量: %.2f GB\n", (size * 512.0) / (1024.0 * 1024.0 * 1024.0));
                log_thread_safe(LOG_LEVEL_INFO, STORAGE_TAG, "udisk path: (%d): %s", usbDiskCount, full_path);
                log_thread_safe(LOG_LEVEL_INFO, STORAGE_TAG, "udisk size: (%d): %.2f GB", usbDiskCount, (size * 512.0) / (1024.0 * 1024.0 * 1024.0));
                usbDiskSizeList[++usbDiskCount] = (size * 512.0) / (1024.0 * 1024.0 * 1024.0);
                g_size_found = true; // 标记已找到size文件
            }
            close(fd);
            closedir(dir);
            return; // 找到后立即返回
        }
    }
    closedir(dir);
}
  

void Storage::scan_usb_with_libusb() {
    usbDiskCount = 0;
    facilityUsbCount3_0 = 0;
    facilityUsbCount2_0 = 0;

    for (int i = 0; i < facilityUsbInfoList3_0.size(); i++) {
        facilityUsbInfoList3_0[i].vid = 0;
        facilityUsbInfoList3_0[i].pid = 0;
    }

    for (int i = 0; i < facilityUsbInfoList2_0.size(); i++) {
        facilityUsbInfoList2_0[i].vid = 0;
        facilityUsbInfoList2_0[i].pid = 0;
    }

    libusb_device** devs;
    libusb_context* ctx = NULL;
    ssize_t cnt;
    int r, i;
    
    r = libusb_init(&ctx);
    if (r < 0) {
        log_thread_safe(LOG_LEVEL_ERROR, STORAGE_TAG, "init libusb failed");
        return;
    }
    
    cnt = libusb_get_device_list(ctx, &devs);
    if (cnt < 0) {
        log_thread_safe(LOG_LEVEL_ERROR, STORAGE_TAG, "get device list failed");
        libusb_exit(ctx);
        return;
    }
    
    log_thread_safe(LOG_LEVEL_INFO, STORAGE_TAG, "found %ld usb devices", cnt);
    
    for (i = 0; i < cnt; i++) {
        libusb_device* dev = devs[i];
        struct libusb_device_descriptor desc;
        libusb_device_handle* handle = NULL;
        unsigned char manufacturer[256] = {0};
        unsigned char product[256] = {0};
        unsigned char serial[256] = {0};
        char type_desc[100] = {0};
        int r;
        
        r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            continue;
        }
        
        // 获取详细设备类型
        get_detailed_device_type(dev, desc.bDeviceClass, desc.bDeviceSubClass, 
                                desc.bDeviceProtocol, type_desc);
        
        // printf("设备 %d:\n", i+1);
        // printf("  Vendor ID: 0x%04x\n", desc.idVendor);
        // printf("  Product ID: 0x%04x\n", desc.idProduct);
        // printf("  设备类: 0x%02x (%s)\n", desc.bDeviceClass, get_device_class_name(desc.bDeviceClass));
        // printf("  设备子类: 0x%02x\n", desc.bDeviceSubClass);
        // printf("  设备协议: 0x%02x\n", desc.bDeviceProtocol);
        // printf("  设备类型: %s\n", type_desc);
        
        // USB版本信息
        // printf("  USB版本: %d.%d\n", desc.bcdUSB >> 8, desc.bcdUSB & 0xFF);
        
        // 尝试打开设备获取详细信息
        r = libusb_open(dev, &handle);
        if (r == 0) {
            // 获取制造商字符串
            if (desc.iManufacturer) {
                r = libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, 
                                                      manufacturer, sizeof(manufacturer));
                // if (r > 0) {
                //     printf("  制造商: %s\n", manufacturer);
                // }
            }
            
            // 获取产品字符串
            if (desc.iProduct) {
                r = libusb_get_string_descriptor_ascii(handle, desc.iProduct, 
                                                      product, sizeof(product));
                // if (r > 0) {
                //     printf("  产品: %s\n", product);
                // }
            }
            
            // 获取序列号
            if (desc.iSerialNumber) {
                r = libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, 
                                                      serial, sizeof(serial));
                // if (r > 0) {
                //     printf("  序列号: %s\n", serial);
                // }
            }
            
            libusb_close(handle);
        }
        
        // 获取总线号和设备地址
        // printf("  总线: %d, 设备: %d\n", 
        //        libusb_get_bus_number(dev), 
        //        libusb_get_device_address(dev));
        
        // 如果是存储设备，查找设备节点和挂载路径并显示容量
        if (is_mass_storage_device_by_interface(dev)) {
            // printf("  [存储设备] 可安全移除\n");

            DIR* dir = opendir("/sys/bus/usb/devices/");
            if (!dir) {
                perror("opendir failed");
                return;
            }
            struct dirent* entry;
            while ((entry = readdir(dir)) != NULL) {
                // 跳过.和..
                if (entry->d_name[0] == '.')
                    continue;
                
                // 构建设备路径
                char dev_path[PATH_MAX];
                snprintf(dev_path, sizeof(dev_path), 
                        "/sys/bus/usb/devices/%s", entry->d_name);
                
                // 读取VID和PID
                char vid_path[PATH_MAX], pid_path[PATH_MAX];
                snprintf(vid_path, sizeof(vid_path), "%s/idVendor", dev_path);
                snprintf(pid_path, sizeof(pid_path), "%s/idProduct", dev_path);

                int fd_vid = open(vid_path, O_RDONLY);
                int fd_pid = open(pid_path, O_RDONLY);
                if (fd_vid < 0 || fd_pid < 0) {
                    if (fd_vid >= 0) close(fd_vid);
                    if (fd_pid >= 0) close(fd_pid);
                    continue;
                }
                char vid_str[10], pid_str[10];
                read(fd_vid, vid_str, sizeof(vid_str)-1);
                read(fd_pid, pid_str, sizeof(pid_str)-1);
                close(fd_vid);
                close(fd_pid);

                uint16_t found_vid = (uint16_t)strtoul(vid_str, NULL, 16);
                uint16_t found_pid = (uint16_t)strtoul(pid_str, NULL, 16);
                if (found_vid == desc.idVendor && found_pid == desc.idProduct) {
                    // printf("找到匹配的USB设备: %s\n", entry->d_name);
                    // printf("VID: 0X%x, PID: 0X%0x\n", found_vid, found_pid);
                    // entry->d_name 目录下递归查找size文件，并计算容量
                    char usb_device_path[PATH_MAX];
                    snprintf(usb_device_path, sizeof(usb_device_path), 
                            "/sys/bus/usb/devices/%s", entry->d_name);
                    // printf("USB设备路径: %s\n", usb_device_path);



                    g_size_found = false; // 重置标志
                    search_directory(usb_device_path);

                    break;
                }
            }
            closedir(dir);
        }
        else if (desc.bDeviceClass == 0x03) {
            // printf("  [输入设备] 键盘/鼠标等\n");
        }
        else if (desc.bDeviceClass == 0x09) {
            // printf("  [集线器] 端口扩展设备\n");
        } else if (desc.bDeviceClass == 0x02 && desc.bDeviceSubClass == 0x02 && desc.bDeviceProtocol == 0x01 && strcmp(reinterpret_cast<const char*>(manufacturer), "WCH") == 0) {
            // printf("发现usb小板\n");
            FacilityusbInfo info;
            info.vid = desc.idVendor;
            info.pid = desc.idProduct;

            char usbTypeBuff[10];
            sprintf(usbTypeBuff, "%d.%d", desc.bcdUSB >> 8, desc.bcdUSB & 0xFF);
            info.usbType = std::string(usbTypeBuff);

            if (info.usbType == "3.0") {              // 这里有效索引从1开始
                facilityUsbInfoList3_0[++facilityUsbCount3_0] = info;
            } else if (info.usbType == "2.0") {
                facilityUsbInfoList2_0[++facilityUsbCount2_0] = info;
            }

        } else {
            // printf("  [其他设备] \n");
        }
        
        // printf("\n");
    }
    
    libusb_free_device_list(devs, 1);
    libusb_exit(ctx);
}

bool Storage::lsusbGetVidPidInfo() {
    lsusbFacilityUsbCount = 0;
    // 清空之前的数据
    for (auto& info : lsusbFacilityUsbInfoList) {
        info.vid = 0;
        info.pid = 0;
    }


    FILE* pipe = popen("lsusb", "r");
    if (!pipe) {
        log_thread_safe(LOG_LEVEL_ERROR, STORAGE_TAG, "popen lsusb failed");
        return false;
    }
    
    char buffer[512];
    std::regex pattern(R"(ID\s+([0-9a-fA-F]{4}):([0-9a-fA-F]{4}))");
    
    while (fgets(buffer, sizeof(buffer), pipe)) {
        std::string line(buffer);
        std::smatch matches;
        
        if (std::regex_search(line, matches, pattern) && matches.size() == 3) {
            try {
                lsusbFacilityusbInfo info;
                info.vid = static_cast<uint16_t>(std::stoul(matches[1].str(), nullptr, 16));
                info.pid = static_cast<uint16_t>(std::stoul(matches[2].str(), nullptr, 16));
                lsusbFacilityUsbInfoList[++lsusbFacilityUsbCount] = info;
            } catch (...) {
                continue;
            }
        }
    }
    pclose(pipe);

    // 打印结果以验证
    for (int i = 1; i <= lsusbFacilityUsbCount; i++) {
        log_thread_safe(LOG_LEVEL_INFO, STORAGE_TAG, 
            "lsusb 设备 %d: VID=0x%04x, PID=0x%04x", 
            i, 
            lsusbFacilityUsbInfoList[i].vid, 
            lsusbFacilityUsbInfoList[i].pid);
    }

    return lsusbFacilityUsbCount > 0;
}