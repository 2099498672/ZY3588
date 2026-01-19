#include "hardware/Fan.h"

Fan::Fan(const char* ctrlPath, FAN_CTRL initialStatus) 
    : FAN_CTRL_PATH(ctrlPath), fanStatus(FAN_OFF), fanFd(-1) {
    fanFd = open(FAN_CTRL_PATH, O_WRONLY);
    if (fanFd < 0) {
        log_thread_safe(LOG_LEVEL_ERROR, FAN_TAG, "cannot open %s", FAN_CTRL_PATH);
        fanFd = -1;
    }
    
    if (!fanCtrl(initialStatus)) {
        log_thread_safe(LOG_LEVEL_ERROR, FAN_TAG, "Closing fan control file descriptor");
        close(fanFd);
        fanFd = -1;
    }
}

Fan::~Fan() {
    if (fanFd >= 0) {
        close(fanFd);
    }
}

bool Fan::fanCtrl(FAN_CTRL status) {
    if (fanFd < 0) {
        log_thread_safe(LOG_LEVEL_ERROR, FAN_TAG, "Fan control file descriptor is invalid");
        return false;
    }

    if (status != FAN_OFF && status != FAN_ON) {
        log_thread_safe(LOG_LEVEL_ERROR, FAN_TAG, "Invalid fan status: %d", status);
        return false;
    }

    const char* value = (status == FAN_ON) ? "on" : "off";
    ssize_t len = (status == FAN_ON) ? 2 : 3;
    
    ssize_t written = write(fanFd, value, len);
    if (written != len) {
        log_thread_safe(LOG_LEVEL_ERROR, FAN_TAG, "Failed to write %s to %s", value, FAN_CTRL_PATH);
        return false;
    }
    
    fanStatus = status;
    log_thread_safe(LOG_LEVEL_INFO, FAN_TAG, "Fan status set to: %s", (status == FAN_ON) ? "ON" : "OFF");
    return true;
}
