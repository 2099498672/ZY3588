#include "hardware/Bluetooth.h"

Bluetooth::Bluetooth() {

}

Bluetooth::~Bluetooth() {
    freeScanResult();
    // log_thread_safe(LOG_LEVEL_INFO, BLUETOOTH_TAG, "Bluetooth deinitialized");
}

bool Bluetooth::scanBluetoothDevices(int duration) {
    freeScanResult();

    // 1. 获取蓝牙适配器ID并打开套接字
    int dev_id = hci_get_route(NULL);
    if (dev_id < 0) {
        // LogError(BLUETOOTH_TAG, "Adapter unavailable: %s", strerror(errno));
        log_thread_safe(LOG_LEVEL_ERROR, BLUETOOTH_TAG, "Adapter unavailable: %s", strerror(errno));
        return false;
    }

    int sock = hci_open_dev(dev_id);
    if (sock < 0) {
        // LogError(BLUETOOTH_TAG, "Open socket failed: %s", strerror(errno));
        log_thread_safe(LOG_LEVEL_ERROR, BLUETOOTH_TAG, "Open socket failed: %s", strerror(errno));
        return false;
    }

    // 2. 执行蓝牙扫描（获取设备BD地址列表）
    int max_rsp = 255;
    int flags = IREQ_CACHE_FLUSH;
    inquiry_info *ii = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));
    if (!ii) {
        // LogError(BLUETOOTH_TAG, "Malloc inquiry_info failed: %s", strerror(errno));
        log_thread_safe(LOG_LEVEL_ERROR, BLUETOOTH_TAG, "Malloc inquiry_info failed: %s", strerror(errno));
        close(sock);
        return false;
    }

    int num_rsp = hci_inquiry(dev_id, duration, max_rsp, NULL, &ii, flags);
    if (num_rsp < 0) {
        // LogError(BLUETOOTH_TAG, "Scan failed: %s", strerror(errno));
        log_thread_safe(LOG_LEVEL_ERROR, BLUETOOTH_TAG, "Scan failed: %s", strerror(errno));
        free(ii);
        close(sock);
        return false;
    } else if (num_rsp == 0) {
        // LogInfo(BLUETOOTH_TAG, "No devices found");
        log_thread_safe(LOG_LEVEL_INFO, BLUETOOTH_TAG, "No devices found");
        free(ii);
        close(sock);
        bluetoothScanResult.count = 0;
        return true;
    }

    // 3. 分配设备数组内存
    bluetoothScanResult.devices = (BluetoothDevice*)malloc(num_rsp * sizeof(BluetoothDevice));
    if (!bluetoothScanResult.devices) {
        // LogError(BLUETOOTH_TAG, "Malloc BluetoothDevice failed: %s", strerror(errno));
        log_thread_safe(LOG_LEVEL_ERROR, BLUETOOTH_TAG, "Malloc BluetoothDevice failed: %s", strerror(errno));
        free(ii);
        close(sock);
        return false;
    }
    bluetoothScanResult.count = num_rsp;

    // 4. 遍历设备：获取MAC、名称、RSSI（已删除冗余的hci_conn_info_t变量）
    for (int i = 0; i < num_rsp; i++) {
        inquiry_info &dev_info = ii[i];
        BluetoothDevice &device = bluetoothScanResult.devices[i];
        int8_t rssi_val = -127; // 默认未知RSSI
        uint16_t acl_handle = 0; // ACL连接句柄

        // 4.1 填充MAC地址
        ba2str(&(dev_info.bdaddr), device.macAddress);

        // 4.2 填充设备名称
        memset(device.name, 0, sizeof(device.name));
        if (hci_read_remote_name(sock, &dev_info.bdaddr, sizeof(device.name), device.name, 1000) < 0) {
            strncpy(device.name, "unknown", sizeof(device.name)-1);
        }

        // 4.3 建立临时ACL连接（无冗余变量）
        if (hci_create_connection(sock, &dev_info.bdaddr, ACL_LINK, 0, 0, &acl_handle, 1000) == 0) {
            // LogDebug(BLUETOOTH_TAG, "ACL connected to %s, handle: %d", device.macAddress, acl_handle);
            log_thread_safe(LOG_LEVEL_DEBUG, BLUETOOTH_TAG, "ACL connected to %s, handle: %d", device.macAddress, acl_handle);

            // 4.4 读RSSI
            if (hci_read_rssi(sock, acl_handle, &rssi_val, 1000) < 0) {
                // LogWarn(BLUETOOTH_TAG, "Read RSSI failed for %s: %s", device.macAddress, strerror(errno));
                log_thread_safe(LOG_LEVEL_WARN, BLUETOOTH_TAG, "Read RSSI failed for %s: %s", device.macAddress, strerror(errno));
                rssi_val = -127;
            } else {
                // LogDebug(BLUETOOTH_TAG, "%s RSSI: %d dBm", device.macAddress, rssi_val);
                log_thread_safe(LOG_LEVEL_DEBUG, BLUETOOTH_TAG, "%s RSSI: %d dBm", device.macAddress, rssi_val);
            }

            // 4.5 断开连接
            if (hci_disconnect(sock, acl_handle, HCI_OE_USER_ENDED_CONNECTION, 1000) < 0) {
                // LogWarn(BLUETOOTH_TAG, "Disconnect %s failed: %s", device.macAddress, strerror(errno));
                log_thread_safe(LOG_LEVEL_WARN, BLUETOOTH_TAG, "Disconnect %s failed: %s", device.macAddress, strerror(errno));
            }
        } else {
            // LogWarn(BLUETOOTH_TAG, "ACL connect to %s failed: %s", device.macAddress, strerror(errno));
            log_thread_safe(LOG_LEVEL_WARN, BLUETOOTH_TAG, "ACL connect to %s failed: %s", device.macAddress, strerror(errno));
        }

        // 4.6 保存RSSI
        device.rssi = rssi_val;
    }

    // 5. 清理资源
    free(ii);
    close(sock);
    // LogInfo(BLUETOOTH_TAG, "Scan done, %d devices found", num_rsp);
    log_thread_safe(LOG_LEVEL_INFO, BLUETOOTH_TAG, "Scan done, %d devices found", num_rsp);
    return true;
}

void Bluetooth::printScanResult() {
    if (bluetoothScanResult.count < 0) {
        // LogError(BLUETOOTH_TAG, "No valid scan result");
        log_thread_safe(LOG_LEVEL_ERROR, BLUETOOTH_TAG, "No valid scan result");
        return;
    }
    if (bluetoothScanResult.count == 0) {
        // LogInfo(BLUETOOTH_TAG, "No Bluetooth devices found");
        log_thread_safe(LOG_LEVEL_INFO, BLUETOOTH_TAG, "No Bluetooth devices found");
        return;
    }

    // LogInfo(BLUETOOTH_TAG, "Scan result: Total %d Bluetooth devices", bluetoothScanResult.count);
    log_thread_safe(LOG_LEVEL_INFO, BLUETOOTH_TAG, "Scan result: Total %d Bluetooth devices", bluetoothScanResult.count);
    for (int i = 0; i < bluetoothScanResult.count; i++) {
        BluetoothDevice &dev = bluetoothScanResult.devices[i];
        // printf("Device %d:\n", i + 1);
        // printf("  MAC Address: %s\n", dev.macAddress);
        // printf("  Name:        %s\n", dev.name);
        // printf("  RSSI:        %d dBm\n", dev.rssi);
        // printf("------------------------------------------\n");
        log_thread_safe(LOG_LEVEL_INFO, BLUETOOTH_TAG,
            "device %d: MAC Address: %s, Name: %s, RSSI: %d dBm",
            i + 1, dev.macAddress, dev.name, dev.rssi);
    }
}

void Bluetooth::freeScanResult() {
    if (bluetoothScanResult.devices != NULL) {
        free(bluetoothScanResult.devices);
        bluetoothScanResult.devices = NULL;
    }
    bluetoothScanResult.count = -1;
}