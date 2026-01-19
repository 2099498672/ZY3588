#include "hardware/Gpio.h"

bool Gpio::gpioExport(int gpioNum) {
    return true;
}

bool Gpio::gpioExport(const char* GPIO_NUM) {
    const char* GPIO_EXPORT_PATH = "/sys/class/gpio/export";

    int fd = -1;
    fd = open(GPIO_EXPORT_PATH, O_WRONLY);
    if (fd < 0) {
        LogError(GPIO_TAG, "%s : open %s failed. ret = %d", strerror(errno), GPIO_EXPORT_PATH, fd);
        return false;
    }

    int ret = -1;
    //LogInfo(GPIO_TAG, GPIO_NUM);
    ret = write(fd, GPIO_NUM, strlen(GPIO_NUM));
    if (ret < 0) {
        LogError(GPIO_TAG, "%s : export %s failed. ret = %d", strerror(errno), GPIO_NUM, ret);
        return false;
    }

    return true;
}

bool Gpio::gpioSetDir(const char* GPIO_PATH, const char* DIR) {
    std::string gpioDirectionStr = std::string(GPIO_PATH) + "/direction";
    const char* GPIO_DIRECTION_PATH = gpioDirectionStr.c_str();

    int fd = -1;
    fd = open(GPIO_DIRECTION_PATH, O_WRONLY);
    if (fd < 0) {
        LogError(GPIO_TAG, "%s : open %s failed. ret = %d", strerror(errno), GPIO_DIRECTION_PATH, fd);
        return false;
    }

    int ret = -1;
    ret = write(fd, DIR, strlen(DIR));
    if (ret < 0) {
        LogError(GPIO_TAG, "%s : set gpio dir %s failed. ret = %d", strerror(errno), DIR, ret);
        return false;
    }

    return true;
}

bool Gpio::gpioGetDir(const char* GPIO_PATH, std::string& dir) {
    std::string gpioDirectionStr = std::string(GPIO_PATH) + "/direction";
    const char* GPIO_DIRECTION_PATH = gpioDirectionStr.c_str();

    char buffer[16] = {0};
    int fd = -1;
    fd = open(GPIO_DIRECTION_PATH, O_RDONLY);
    if (fd < 0) {
        LogError(GPIO_TAG, "%s : open %s failed. ret = %d", strerror(errno), GPIO_DIRECTION_PATH, fd);
        return false;
    }

    int ret = -1;
    ret = read(fd, buffer, sizeof(buffer));
    if (ret < 0) {
        LogError(GPIO_TAG, "%s : get gpio dir failed. ret = %d", strerror(errno), ret);
        return false;
    }

    dir = std::string(buffer);
    return true;
}

bool Gpio::gpioSetValue(int gpioNum, int value) {     // gpioNum 127   value 1
    std::string gpioNumStr = std::to_string(gpioNum);
    std::string valueStr = std::to_string(value);
    const char* GPIO_NUM = gpioNumStr.c_str();     // 127
    const char* GPIO_VALUE = valueStr.c_str();     // 1
    const char* GPIO_DIRECTION_OUT = "out";
    
    std::string gpioPathStr = "/sys/class/gpio/gpio" + gpioNumStr;
    std::string gpioValuePathStr = gpioPathStr + "/value";
    const char* GPIO_PATH = gpioPathStr.c_str();
    const char* GPIO_VALUE_PATH = gpioValuePathStr.c_str();

    if (access(GPIO_PATH, F_OK) != 0) {
        if (!gpioExport(GPIO_NUM)) {
            return false;
        }
    } else {
        LogInfo(GPIO_TAG, "gpio %d already exported", gpioNum);
    }

    if (!gpioSetDir(GPIO_PATH, GPIO_DIRECTION_OUT)) {
        return false;
    }

    std::string gpioDir;
    if (gpioGetDir(GPIO_PATH, gpioDir) == false || gpioDir.find(GPIO_DIRECTION_OUT) == std::string::npos) {
        return false;
    }

    int fd = -1;
    fd = open(GPIO_VALUE_PATH, O_WRONLY);
    if (fd < 0) {
        LogError(GPIO_TAG, "%s : open %s failed. ret = %d", strerror(errno), GPIO_VALUE_PATH, fd);
        return false;
    }

    int ret = -1;
    ret = write(fd, GPIO_VALUE, strlen(GPIO_VALUE));
    if (ret < 0) {
        LogError(GPIO_TAG, "%s : set gpio value %s failed. ret = %d", strerror(errno), GPIO_VALUE, ret);
        return false;
    }
    LogInfo(GPIO_TAG, "set gpio %d value %d success", gpioNum, value);
    return true;
}

bool Gpio::gpioGetValue(int gpioNum, int value) {
    std::string gpioNumStr = std::to_string(gpioNum);
    std::string valueStr = std::to_string(value);
    const char* GPIO_NUM = gpioNumStr.c_str();     // 127
    const char* GPIO_VALUE = valueStr.c_str();     // 1
    const char* GPIO_DIRECTION_IN = "in";

    std::string gpioPathStr = "/sys/class/gpio/gpio" + gpioNumStr;
    std::string gpioValuePathStr = gpioPathStr + "/value";
    const char* GPIO_PATH = gpioPathStr.c_str();
    const char* GPIO_VALUE_PATH = gpioValuePathStr.c_str();

    LogInfo(GPIO_TAG, "GPIO_PATH: %s", GPIO_PATH); 
    if (access(GPIO_PATH, F_OK) != 0) {
        if (!gpioExport(GPIO_NUM)) {
            return false;
        }
    } else {
        LogInfo(GPIO_TAG, "gpio %d already exported", gpioNum);
    }

    if (!gpioSetDir(GPIO_PATH, GPIO_DIRECTION_IN)) {
        return false;
    }

    std::string gpioDir;
    if (gpioGetDir(GPIO_PATH, gpioDir) == false || gpioDir.find(GPIO_DIRECTION_IN) == std::string::npos) {
        return false;
    }

    char buffer[16] = {0};
    int fd = -1;
    fd = open(GPIO_VALUE_PATH, O_RDONLY);
    if (fd < 0) {
        LogError(GPIO_TAG, "%s : open %s failed. ret = %d", strerror(errno), GPIO_VALUE_PATH, fd);
        return false;
    }
    int ret = -1;
    ret = read(fd, buffer, sizeof(buffer));
    if (ret < 0) {
        LogError(GPIO_TAG, "%s : get gpio value failed. ret = %d", strerror(errno), ret);
        return false;
    }

    LogInfo(GPIO_TAG, "read gpio %d value success, read buffer: %s", gpioNum, buffer);

    std::string readValue(buffer);
    if (readValue.find(GPIO_VALUE) != std::string::npos) {
        LogInfo(GPIO_TAG, "gpio %d value %d match success", gpioNum, value);
        return true;
    } else {
        LogInfo(GPIO_TAG, "gpio %d value %d match failed, read value: %s", gpioNum, value, readValue.c_str());
        return false;
    }
}