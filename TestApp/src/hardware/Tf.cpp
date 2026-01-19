#include "hardware/Tf.h"

Tf::Tf(const char* tfCardDeviceSizePath) 
    : TF_CARD_DEVICE_SIZE_PATH(tfCardDeviceSizePath) {
}

float Tf::getTfCardSize() {
    FILE* fp = fopen(TF_CARD_DEVICE_SIZE_PATH, "r");
    if (!fp) {
        LogError(TF_TAG, "can not open : %s", TF_CARD_DEVICE_SIZE_PATH);
        return -1;
    }

    unsigned long long sectors;
    if (fscanf(fp, "%llu", &sectors) != 1) {
        LogError(TF_TAG, "read emmc sectors failed. ");
        fclose(fp);
        return -1;
    }
    fclose(fp);

    unsigned long long totalBytes = sectors * 512;    // 计算总字节数和KB
    unsigned long long totalKb = totalBytes / 1024;   // 转换为KB
    // unsigned long long totalMb = totalBytes / (1024 * 1024);
    
    float totalGb = static_cast<float>(totalBytes) / (1024.0f * 1024.0f * 1024.0f);      // 转换为GB并保留两位小数
    float roundedGb = round(totalGb * 100.0f) / 100.0f;
    
    LogInfo(TF_TAG, "emmc total size: %llu KB (%.2f GB)", totalKb, roundedGb);
    return roundedGb;
} 