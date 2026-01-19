#include "hardware/VendorStorage.h"

void VendorStorage::print_hex_data(const char *s, uint32_t *buf, uint32_t len) {
    uint32_t i;
    printf("%s\n", s);
    for (i = 0; i < len; i += 4) {
        if (i + 3 < len) {
            printf("%02x %02x %02x %02x\n",
                   ((uint8_t*)buf)[i],
                   ((uint8_t*)buf)[i + 1],
                   ((uint8_t*)buf)[i + 2],
                   ((uint8_t*)buf)[i + 3]);
        }
    }
}

int VendorStorage::vendor_storage_read(uint16_t id, uint8_t *data, uint16_t *len) {
    int ret, sys_fd;
    uint8_t p_buf[2048]; /* malloc req buffer or used extern buffer */
    struct rk_vendor_req *req;
    req = (struct rk_vendor_req *)p_buf;
    
    sys_fd = open("/dev/vendor_storage", O_RDWR, 0);
    if (sys_fd < 0) {
        printf("vendor_storage open fail\n");
        return -1;
    }

    req->tag = VENDOR_REQ_TAG;
    req->id = id;
    req->len = 512; /* max read length to read*/
    ret = ioctl(sys_fd, VENDOR_READ_IO, req);
    print_hex_data("vendor read:", (uint32_t*)req, req->len + 8);
    /* return req->len is the real data length stored in the NV-storage */
    if (ret) {
        printf("vendor read error\n");
        close(sys_fd);
        return -1;
    }
    
    /* Copy data to output buffer if provided */
    if (data != NULL && len != NULL) {
        uint16_t copy_len = (req->len < *len) ? req->len : *len;
        memcpy(data, req->data, copy_len);
        *len = req->len; /* Return actual data length */
    }
    
    close(sys_fd);
    return 0;
}

int VendorStorage::vendor_storage_write(uint16_t id, const uint8_t *data, uint16_t len) {
    int ret, sys_fd;
    uint8_t p_buf[2048]; /* malloc req buffer or used extern buffer */
    struct rk_vendor_req *req;
    
    if (len > 2048 - sizeof(struct rk_vendor_req)) {
        printf("Data length too large: %d\n", len);
        return -1;
    }
    
    req = (struct rk_vendor_req *)p_buf;
    
    sys_fd = open("/dev/vendor_storage", O_RDWR, 0);
    if (sys_fd < 0) {
        printf("vendor_storage open fail\n");
        return -1;
    }

    req->tag = VENDOR_REQ_TAG;
    req->id = id;
    req->len = len;
    
    /* Copy data from parameter to request buffer */
    if (data != NULL && len > 0) {
        memcpy(req->data, data, len);
    }

    print_hex_data("vendor write:", (uint32_t*)req, req->len + 8);
    ret = ioctl(sys_fd, VENDOR_WRITE_IO, req);
    if (ret) {
        printf("vendor write error\n");
        close(sys_fd);
        return -1;
    }
    close(sys_fd);
    return 0;
}

int VendorStorage::vendor_storage_write_ln(const char *ln_string) {
    uint16_t len = strlen(ln_string);
    if (len > 512) {
        printf("LN string too long: %d\n", len);
        return -1;
    }
    return vendor_storage_write(VENDOR_CUSTOM_ID_1, (const uint8_t*)ln_string, len); 
}

int VendorStorage::vendor_storage_write_sn(const char *sn_string) {
    uint16_t len = strlen(sn_string);
    if (len > 512) {
        printf("SN string too long: %d\n", len);
        return -1;
    }
    return vendor_storage_write(VENDOR_SN_ID, (const uint8_t*)sn_string, len);
}

int VendorStorage::vendor_storage_write_mac(uint16_t mac_id, const uint8_t *mac_addr, uint8_t addr_len) {
    if (addr_len != 6) {
        printf("MAC address must be 6 bytes\n");
        return -1;
    }
    return vendor_storage_write(mac_id, mac_addr, addr_len);
}