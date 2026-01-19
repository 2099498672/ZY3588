#include "hardware/Led.h"

Led::Led(const char* RLED_BRIGHTNESS_PATH, LED_CTRL rled_status,
         const char* GLED_BRIGHTNESS_PATH, LED_CTRL gled_status)
    : RLED_BRIGHTNESS_PATH(RLED_BRIGHTNESS_PATH),
      rled_status(rled_status),
      GLED_BRIGHTNESS_PATH(GLED_BRIGHTNESS_PATH),
      gled_status(gled_status),
      rledFd(-1),
      gledFd(-1) {

    rledFd = open(RLED_BRIGHTNESS_PATH, O_WRONLY);
    if (rledFd < 0) {
        log_thread_safe(LOG_LEVEL_ERROR, LED_TAG, "cannot open %s", RLED_BRIGHTNESS_PATH);
        rledFd = -1;
    }

    gledFd = open(GLED_BRIGHTNESS_PATH, O_WRONLY);
    if (gledFd < 0) {
        log_thread_safe(LOG_LEVEL_ERROR, LED_TAG, "cannot open %s", GLED_BRIGHTNESS_PATH);
        gledFd = -1;
    }

    if (rledFd >= 0) {
        setRledStatus(rled_status);
    }
    
    if (gledFd >= 0) {
        setGledStatus(gled_status);
    }
}

Led::~Led() {
    if (rledFd >= 0) {
        close(rledFd);
    }
    if (gledFd >= 0) {
        close(gledFd);
    }
}

bool Led::setRledStatus(LED_CTRL status) {
    if (rledFd < 0) {
        log_thread_safe(LOG_LEVEL_ERROR, LED_TAG, "RLED fd is invalid");
        return false;
    }

    const char* value = (status == LED_ON) ? "255" : "0";
    int len = strlen(value);
    ssize_t ret = write(rledFd, value, len);
    if (ret < 0) {
        log_thread_safe(LOG_LEVEL_ERROR, LED_TAG, "Failed to set RLED status");
        return false;
    } else {
        rled_status = status;
        log_thread_safe(LOG_LEVEL_INFO, LED_TAG, "RLED set to %s", (status == LED_ON) ? "ON" : "OFF");
        return true;
    }
}

bool Led::setGledStatus(LED_CTRL status) {
    if (gledFd < 0) {
        log_thread_safe(LOG_LEVEL_ERROR, LED_TAG, "GLED fd is invalid");
        return false;
    }

    const char* value = (status == LED_ON) ? "255" : "0";
    int len = strlen(value);
    ssize_t ret = write(gledFd, value, len);
    if (ret < 0) {
        log_thread_safe(LOG_LEVEL_ERROR, LED_TAG, "Failed to set GLED status");
        return false;
    } else {
        gled_status = status;
        log_thread_safe(LOG_LEVEL_INFO, LED_TAG, "GLED set to %s", (status == LED_ON) ? "ON" : "OFF");
        return true;
    }
}