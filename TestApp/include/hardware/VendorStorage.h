#ifndef __VENDOR_STORAGE_H__
#define __VENDOR_STORAGE_H__

#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include "util/Log.h"

#define VENDOR_REQ_TAG 0x56524551
#define VENDOR_READ_IO _IOW('v', 0x01, unsigned int)
#define VENDOR_WRITE_IO _IOW('v', 0x02, unsigned int)
#define VENDOR_SN_ID 1
#define VENDOR_WIFI_MAC_ID 2
#define VENDOR_LAN_MAC_ID 3
#define VENDOR_BLUETOOTH_ID 4

#define VENDOR_CUSTOM_ID_1 32

struct rk_vendor_req {
    uint32_t tag;
    uint16_t id;
    uint16_t len;
    uint8_t data[1];
};

class VendorStorage {

public:
    const char* VENDOR_STORAGE_TAG = "VENDOR_STORAGE";

    void print_hex_data(const char *s, uint32_t *buf, uint32_t len);
    int vendor_storage_read(uint16_t id, uint8_t *data, uint16_t *len);
    int vendor_storage_write(uint16_t id, const uint8_t *data, uint16_t len);

    int vendor_storage_write_ln(const char *ln_string);
    int vendor_storage_write_sn(const char *sn_string);
    int vendor_storage_write_mac(uint16_t mac_id, const uint8_t *mac_addr, uint8_t addr_len);
};

#endif  // __VENDOR_STORAGE_H__