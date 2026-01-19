#include "hardware/Adc.h"

int Adc::get_adc_value(std::string adc_device_path) {
    int fd = open(adc_device_path.c_str(), O_RDONLY);
    if (fd < 0) {
        log_thread_safe(LOG_LEVEL_ERROR, ADC_TAG, "cannot open adc device: %s", adc_device_path.c_str());
        return -1;
    }

    char buf[16];
    ssize_t len = read(fd, buf, sizeof(buf) - 1);
    if (len < 0) {
        log_thread_safe(LOG_LEVEL_ERROR, ADC_TAG, "get_adc_value read error: %s", strerror(errno));
        close(fd);
        return -1;
    }

    buf[len] = '\0'; // ensure null-terminated string
    std::string adc_value(buf);
    log_thread_safe(LOG_LEVEL_DEBUG, ADC_TAG, "adc device path: %s, raw value: %s", adc_device_path.c_str(), adc_value.c_str());

    int value = std::stoi(adc_value);      // convert string to integer
    
    close(fd);

    return value;
}