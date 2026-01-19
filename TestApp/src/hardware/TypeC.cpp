#include "hardware/TypeC.h"

int TypeC::typeCTest(std::string typeCPath) {
    int fd = open(typeCPath.c_str(), O_RDONLY);
    if (fd < 0) {
        LogError(TYPEC_TAG, "无法打开Type-C设备: %s", typeCPath.c_str());
        return -1;
    }

    char buf[16];
    ssize_t len = read(fd, buf, sizeof(buf) - 1);
    if (len < 0) {
        LogError(TYPEC_TAG, "读取Type-C设备失败: %s", typeCPath.c_str());
        close(fd);
        return -1;
    }
    close(fd);


    buf[len] = '\0'; // 确保字符串以null结尾
    std::string typeCValue(buf);
    LogInfo(TYPEC_TAG, "Type-C设备 %s 读取值: %s", typeCPath.c_str(), typeCValue.c_str());
    // 正插：normal   反插：reverse  未插入或异常：unknown
    if (strstr(buf, "normal") != NULL) {
        LogInfo(TYPEC_TAG, "Type-C设备 %s 正插", typeCPath.c_str());
        return 1;
    } else if (strstr(buf, "reverse") != NULL) {
        LogInfo(TYPEC_TAG, "Type-C设备 %s 反插", typeCPath.c_str());
        return 0;
    } else if (strstr(buf, "unknown") != NULL) {
        LogInfo(TYPEC_TAG, "Type-C设备 %s 未插入或异常", typeCPath.c_str());
        return -1;
    }
    return -1;
}
