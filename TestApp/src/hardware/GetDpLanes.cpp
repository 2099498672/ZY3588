#include "hardware/GetDpLanes.h"

int GetDpLanes::readDpLanes(const char *dpLanesPath) {
    int fd = open(dpLanesPath, O_RDONLY);
    if (fd < 0) {
        LogError(GET_DP_LANES_TAG, "无法打开DP Lanes文件: %s", dpLanesPath);
        return -1;
    }

    char buffer[16] = {0};
    ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    if (bytesRead < 0) {
        LogError(GET_DP_LANES_TAG, "读取DP Lanes文件失败");
        close(fd);
        return -1;
    }
    buffer[bytesRead] = '\0'; // 确保字符串以null结尾
    printf("DP Lanes Raw Data: %s\n", buffer);
    close(fd);
    return std::stoi(buffer);
}