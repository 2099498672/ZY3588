#include "hardware/Wifi.h"

Wifi::Wifi(const char* interface, int scanCount) : INTERFACE(interface), scanCount(scanCount) {
    int commandLength = strlen("wpa_cli -i ") + strlen(interface) + strlen(" scan") + 1;
    scanCommand = new char[commandLength];
    snprintf(scanCommand, commandLength, "wpa_cli -i %s scan", interface);

    int resultsCommandLength = strlen("wpa_cli -i ") + strlen(interface) + strlen(" scan_results") + 1;
    scanResultsCommand = new char[resultsCommandLength];
    snprintf(scanResultsCommand, resultsCommandLength, "wpa_cli -i %s scan_results", interface);

    // LogInfo(WIFI_TAG, "Using interface: %s", INTERFACE);
    // LogInfo(WIFI_TAG, "cmd : %s", scanCommand);
    // LogInfo(WIFI_TAG, "cmd : %s", scanResultsCommand);
    // LogInfo(WIFI_TAG, "Scan count: %d", scanCount);
}

Wifi::~Wifi() {
    delete[] scanCommand;
    delete[] scanResultsCommand;
}

int Wifi::wpaScanWifi(const char* ssid) {
    LogInfo(WIFI_TAG, "Scanning for SSID: %s", ssid);

    while(scanCount--) {
        int level = -100;

        FILE * fp = popen(scanCommand, "r");
        if(fp == NULL) {
            LogError(WIFI_TAG, "popen failed");
            continue;
        }
        char line[1024];
        fgets(line, sizeof(line), fp);
        LogDebug(WIFI_TAG, "scan res : %s", line);
        if(strstr(line, "OK") != NULL) {
            LogInfo(WIFI_TAG, "Scan command executed successfully");
            pclose(fp);
        } else {
            LogError(WIFI_TAG, "Scan command failed");
            pclose(fp);
            sleep(1);
            continue;
        }
        
        fp = popen(scanResultsCommand, "r");
        if(fp == NULL) {
            LogError(WIFI_TAG, "popen failed");
            return -1;
        }

        char bssid[128];
        int frequency;
        char flags[128];
        char ssid_tmp[128];

        while (fgets(line, sizeof(line), fp) != NULL) {
            LogDebug(WIFI_TAG, "%s", line);
            if (strstr(line, ssid)) {
                sscanf(line, "%127s %d %d %127s %127s", bssid, &frequency, &level, flags, ssid_tmp);
                break;
            }
        } 
        pclose(fp);

        if (level > -100) {
            if (level >= -60) {
                return 5;
            } else if (level >= -70) {
                return 4;
            } else if (level >= -80) {
                return 3;
            } else if (level >= -90) {
                return 2;
            } else if (level >= -95) {
                return 1;
            } else {
                return 0;
            }
        }
        sleep(1);
    }
    return -1;
}
