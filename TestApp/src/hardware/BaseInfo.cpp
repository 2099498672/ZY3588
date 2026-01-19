#include "hardware/BaseInfo.h"

BaseInfo::BaseInfo(const char* hwid_path) : HWID_PATH(hwid_path) {

}

bool BaseInfo::get_hwid(std::string& hwid) {
    FILE* fp = fopen(HWID_PATH, "r");
    if (!fp) {
        log_thread_safe(LOG_LEVEL_ERROR, BASEINFO_TAG, "cannot open hwid path: %s", HWID_PATH);
        return false;
    }

    char buffer[256];
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        hwid = std::string(buffer);
        hwid.erase(hwid.find_last_not_of("\n\r") + 1);  // Remove trailing newline characters
        fclose(fp);
        log_thread_safe(LOG_LEVEL_INFO, BASEINFO_TAG, "read hwid: %s", hwid.c_str());
        return true;
    } else {
        log_thread_safe(LOG_LEVEL_ERROR, BASEINFO_TAG, "failed to read hwid from: %s", HWID_PATH);
        fclose(fp);
        return false;
    }
}

void BaseInfo::set_firmware_version(const std::string& firmware_version) {
    this->firmware_version = firmware_version;
}

std::string BaseInfo::get_firmware_version() {
    return firmware_version;
}

bool BaseInfo::get_app_version(std::string& app_version) {
    return false;
}