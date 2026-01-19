#include <cstring>
#include <dirent.h>
#include <sys/ioctl.h>
#include <vector>

#include "hardware/Key.h"

Key::Key() :timeOut(60), fd(-1) {

}

Key::Key(const char* eventPath, int timeOut) : KEY_EVENT_PATH(eventPath), timeOut(timeOut), fd(-1) {

}

// wait for key press from multiple input devices
// return the key code if a key is pressed within the timeout period, otherwise return -1
// timeOut: timeout in seconds
int Key::waitForKeyPress(int timeOut) {
    if (keyEventNamesToPath.empty()) {
        log_thread_safe(LOG_LEVEL_ERROR, KEY_TAG, "Key event names to path map is empty.");
        return -1;
    }

    for (const auto& pair : keyEventNamesToPath) {                                                  // print key event names to listen for
        const std::string& eventName = pair.first;
        log_thread_safe(LOG_LEVEL_INFO, KEY_TAG, "Listening for key event: %s", eventName.c_str());
    }

    DIR *dir;
    struct dirent *entry;
    int temp_fd = -1;
    char device_name[512] = {0};
    char device_path[512] = {0};
    dir = opendir(DEV_INPUT);                                                                       // open "/dev/input"
    if (dir == NULL) {
        log_thread_safe(LOG_LEVEL_ERROR, KEY_TAG, "can not open input device directory: %s", DEV_INPUT);
        return -1;
    }

    int keyEventCount = 0;
    log_thread_safe(LOG_LEVEL_INFO, KEY_TAG, "Scanning input device directory: %s", DEV_INPUT);
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, EVENT_DEV, strlen(EVENT_DEV)) == 0) {                            // device name starts with "event"
            memset(device_name, 0, sizeof(device_name));                                            // clear device name buffer
            memset(device_path, 0, sizeof(device_path));                                            // clear device path buffer
            snprintf(device_path, sizeof(device_path), "%s/%s", DEV_INPUT, entry->d_name);          // construct device path

            temp_fd = open(device_path, O_RDONLY);
            if (temp_fd == -1) {
                log_thread_safe(LOG_LEVEL_WARN, KEY_TAG, "can not open device file: %s", device_path);
                continue;
            }

            if (ioctl(temp_fd, EVIOCGNAME(sizeof(device_name)), device_name) < 0) {
                log_thread_safe(LOG_LEVEL_WARN, KEY_TAG, "ioctl EVIOCGNAME failed for device: %s", device_path);
                close(temp_fd);
                continue;
            }

            log_thread_safe(LOG_LEVEL_INFO, KEY_TAG, "check device : %s -> %s", device_name, device_path);
            for (auto& pair : keyEventNamesToPath) {
                const std::string& eventName = pair.first;
                if (strstr(device_name, eventName.c_str()) != NULL) {
                    log_thread_safe(LOG_LEVEL_INFO, KEY_TAG, "already mached event name : %s  event path : %s", device_name, device_path);
                    pair.second = device_path;                                                  // store matched device path
                    keyEventCount++;
                }
            }
        }
    }

    log_thread_safe(LOG_LEVEL_INFO, KEY_TAG, "needed key event device count: %d", keyEventCount);
    if (keyEventCount != keyEventNamesToPath.size()) {
        log_thread_safe(LOG_LEVEL_ERROR, KEY_TAG, "already found key event device count (%d) does not match needed count (%d)", keyEventCount, (int)keyEventNamesToPath.size());
        log_thread_safe(LOG_LEVEL_ERROR, KEY_TAG, "please check if the target key event devices are present");
        closedir(dir);
        return -1;
    }
    closedir(dir);

   // TODO : The code above and the code below can be modified to two separate functions.

    int device_fds[16];
    int device_count = 0;
    int maxFd = -1;
    bool all_opened = true;
    for (const auto& pair : keyEventNamesToPath) {
        if (device_count >= 16) {
            log_thread_safe(LOG_LEVEL_WARN, KEY_TAG, "device count exceeds maximum limit of 16, ignoring extra devices");
            break;
        }
        const std::string& eventPath = pair.second;
        device_fds[device_count] = open(eventPath.c_str(), O_RDONLY);                           // store device file descriptor
        maxFd = (device_fds[device_count] > maxFd) ? device_fds[device_count] : maxFd;          // update maxFd
        if (device_fds[device_count] < 0) {
            log_thread_safe(LOG_LEVEL_ERROR, KEY_TAG, "can not open target device file: %s", eventPath.c_str());
            all_opened = false;
        }
        device_count++;
    }

    if (!all_opened) {
        log_thread_safe(LOG_LEVEL_ERROR, KEY_TAG, "Failed to open all target device files");
        for (int i = 0; i < device_count; i++) {
            if (device_fds[i] >= 0) {
                close(device_fds[i]);
            }
        }
        return -1;
    }

    struct input_event ev;
    fd_set readfds;
    FD_ZERO(&readfds);                                                                          // clear the set
    for(int i = 0; i < device_count; i++) {
        FD_SET(device_fds[i], &readfds);
    }

    struct timeval timeout;                                                                    // set timeout
    timeout.tv_sec = timeOut;
    timeout.tv_usec = 0;
    log_thread_safe(LOG_LEVEL_INFO, KEY_TAG, "Listening for key events, timeout: %d seconds...", timeOut);
    int ret = select(maxFd + 1, &readfds, NULL, NULL, &timeout);                               // wait for event or timeout
    if (ret == -1) {
        log_thread_safe(LOG_LEVEL_ERROR, KEY_TAG, "select error: %s", strerror(errno));
        for (int i = 0; i < device_count; i++) {                                               // close all device fds
            close(device_fds[i]);
        }
        return -1;
    } else if (ret == 0) {
        log_thread_safe(LOG_LEVEL_INFO, KEY_TAG, "Timeout, no key pressed within %d seconds", timeOut);
        for (int i = 0; i < device_count; i++) {
            close(device_fds[i]);
        }
        return 0;
    }

    int pressedKeyCode = -1;
    for (int i = 0; i < device_count; i++) {                                                // check which device has event
        if (FD_ISSET(device_fds[i], &readfds)) {                                            // device has event
            ssize_t n = read(device_fds[i], &ev, sizeof(ev));                               // read event
            if (n == -1) {
                log_thread_safe(LOG_LEVEL_ERROR, KEY_TAG, "read event failed for device fd %d: %s", device_fds[i], strerror(errno));
                continue;
            }

            if (ev.type == EV_KEY) {                                                          // is key event
                if (ev.value == 1 || ev.value == 0) {                                                          // key press event
                    log_thread_safe(LOG_LEVEL_INFO, KEY_TAG, "Detected key press - Key code: %d", ev.code);
                    pressedKeyCode = ev.code;
                    break;                                                                    // exit loop after first key press detected  ???????
                }
            } 
        }
    }
    for (int i = 0; i < device_count; i++) {
        close(device_fds[i]);
    }
    return pressedKeyCode;
}

int Key::waitForKeyAndIrPress(const char* keyPath, const char* irKeyPath, int timeOut) {
    int keyFd = open(keyPath, O_RDONLY);
    if (keyFd < 0) {
        LogError(KEY_TAG, "Failed to open key event path: %s", keyPath);
        return -1;
    }

    int irFd = open(irKeyPath, O_RDONLY);
    if (irFd < 0) {
        LogError(KEY_TAG, "Failed to open IR key event path: %s", irKeyPath);
        close(keyFd);
        return -1;
    }

    struct input_event ev;
    fd_set readfds;
    struct timeval timeout;

    // 初始化文件描述符集
    FD_ZERO(&readfds);
    FD_SET(keyFd, &readfds);
    FD_SET(irFd, &readfds);
    int maxFd = (keyFd > irFd) ? keyFd : irFd;

    // 设置超时时间
    timeout.tv_sec = timeOut;
    timeout.tv_usec = 0;

    printf("开始监听按键和红外按键，超时时间: %d秒...\n", timeOut);

    // 使用select等待事件或超时
    int ret = select(maxFd + 1, &readfds, NULL, NULL, &timeout);
    if (ret == -1) {
        perror("select错误");
        close(keyFd);
        close(irFd);
        return -1;
    } 
    else if (ret == 0) {
        // 超时
        printf("超时，%d秒内没有按键或红外按键按下\n", timeOut);
        close(keyFd);
        close(irFd);
        return -1;
    }

    int activeFd = -1;
    if (FD_ISSET(keyFd, &readfds)) {
        activeFd = keyFd;
    } else if (FD_ISSET(irFd, &readfds)) {
        activeFd = irFd;
    }

    // 有事件发生，读取事件
    ssize_t n = read(activeFd, &ev, sizeof(ev));
    if (n == -1) {
        perror("读取事件失败");
        close(keyFd);
        close(irFd);
        return -1;
    }

    // 判断是否为按键事件
    if (ev.type == EV_KEY) {
        // 按键按下事件
        if (ev.value == 1) {
            if (activeFd == keyFd) {
                printf("检测到按键按下 - 按键码: %d\n", ev.code);
            } else {
                printf("检测到红外按键按下 - 按键码: %d\n", ev.code);
            }
        }
    }
    close(keyFd);
    close(irFd);
    return ev.code;
}

// int Key::waitForKeyAndIrPress(const char* keyPath, const char* irKeyPath, int timeOut) {
//     int keyFd = open(keyPath, O_RDONLY);
//     if (keyFd < 0) {
//         LogError(KEY_TAG, "Failed to open key event path: %s", keyPath);
//         return -1;
//     }

//     int irFd = open(irKeyPath, O_RDONLY);
//     if (irFd < 0) {
//         LogError(KEY_TAG, "Failed to open IR key event path: %s", irKeyPath);
//         close(keyFd);
//         return -1;
//     }

//     struct input_event ev;
//     fd_set readfds;
//     struct timeval timeout;

//     // 初始化文件描述符集
//     FD_ZERO(&readfds);
//     FD_SET(keyFd, &readfds);
//     FD_SET(irFd, &readfds);
//     int maxFd = (keyFd > irFd) ? keyFd : irFd;

//     // 设置超时时间
//     timeout.tv_sec = timeOut;
//     timeout.tv_usec = 0;

//     printf("开始监听按键和红外按键，超时时间: %d秒...\n", timeOut);

//     // 使用select等待事件或超时
//     int ret = select(maxFd + 1, &readfds, NULL, NULL, &timeout);
//     if (ret == -1) {
//         perror("select错误");
//         close(keyFd);
//         close(irFd);
//         return -1;
//     } 
//     else if (ret == 0) {
//         // 超时
//         printf("超时，%d秒内没有按键或红外按键按下\n", timeOut);
//         close(keyFd);
//         close(irFd);
//         return -1;
//     }

//     int activeFd = -1;
//     if (FD_ISSET(keyFd, &readfds)) {
//         activeFd = keyFd;
//     } else if (FD_ISSET(irFd, &readfds)) {
//         activeFd = irFd;
//     }

//     // 有事件发生，读取事件
//     ssize_t n = read(activeFd, &ev, sizeof(ev));
//     if (n == -1) {
//         perror("读取事件失败");
//         close(keyFd);
//         close(irFd);
//         return -1;
//     }

//     // 判断是否为按键事件
//     if (ev.type == EV_KEY) {
//         // 按键按下事件
//         if (ev.value == 1) {
//             if (activeFd == keyFd) {
//                 printf("检测到按键按下 - 按键码: %d\n", ev.code);
//             } else {
//                 printf("检测到红外按键按下 - 按键码: %d\n", ev.code);
//             }
//         }
//     }
//     close(keyFd);
//     close(irFd);
//     return ev.code;
// }

// wait for key press v3.0, not yet in use
void Key::set_event_info_map(const std::map<std::string, event_info> info_map) {
    event_info_map = info_map;
}

bool Key::get_event_info() {
    const char* dev_input = "/dev/input";
    const char* event_prefix = "event";

    DIR *dir;
    dir = opendir(dev_input);                                                                     
    if (dir == NULL) {
        log_thread_safe(LOG_LEVEL_ERROR, KEY_TAG, "can not open input device directory: %s", dev_input);
        return false;
    }

    struct dirent *entry;
    char event_name[128] = {0};
    char device_path[128] = {0};
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, event_prefix, strlen(event_prefix)) != 0) {
            continue;                                                                                      // device name does not start with "event"
        }
        memset(event_name, 0, sizeof(event_name));                                                    // clear event name buffer
        memset(device_path, 0, sizeof(device_path));                                                  // clear device path buffer
        sprintf(device_path, "%s/%s", dev_input, entry->d_name);                                    // construct device path                

        int fd = open(device_path, O_RDONLY);
        if (fd < 0) {
            log_thread_safe(LOG_LEVEL_WARN, KEY_TAG, "can not open device file: %s", device_path);
            continue;
        }

        if (ioctl(fd, EVIOCGNAME(sizeof(event_name)), event_name) < 0) {
            log_thread_safe(LOG_LEVEL_WARN, KEY_TAG, "ioctl EVIOCGNAME failed for device: %s", entry->d_name);
            close(fd);
            continue;
        }

        log_thread_safe(LOG_LEVEL_INFO, KEY_TAG, "Found device : %s -> %s", event_name, device_path);

        auto it = event_info_map.find(event_name);
        if (it == event_info_map.end()) {
            log_thread_safe(LOG_LEVEL_INFO, KEY_TAG, "Event device %s not in the target list, skipping", event_name);
            close(fd);
            continue;
        } else {
            log_thread_safe(LOG_LEVEL_INFO, KEY_TAG, "Event device %s matched, opening", event_name);
        }
        auto& info = it->second;

        info.fd = fd;
        info.device_path = std::string(dev_input) + "/" + entry->d_name;
        info.is_open = true;
    }
    closedir(dir);

    for (const auto& pair : event_info_map) {
        if (!pair.second.is_open) {
            log_thread_safe(LOG_LEVEL_ERROR, KEY_TAG, "Event device %s not found", pair.first.c_str());
            return false;
        }
    }
    log_thread_safe(LOG_LEVEL_INFO, KEY_TAG, "All event devices opened successfully");
    return true;
}

std::map<std::string, event_info> Key::get_event_info_map() {
    return event_info_map;
}

void Key::print_event_info_map() {
    log_thread_safe(LOG_LEVEL_INFO, KEY_TAG, "Printing event info map:");
    for (const auto& pair : event_info_map) {
        const auto& info = pair.second;
        log_thread_safe(LOG_LEVEL_INFO, KEY_TAG, "Event Name: %s, Device Path: %s, FD: %d, Is Open: %s",
                        pair.first.c_str(),
                        info.device_path.c_str(),
                        info.fd,
                        info.is_open ? "true" : "false");
    }
}

int Key::wait_key_press(int time_out) {
    fd_set read_fds;
    FD_ZERO(&read_fds);                                                                          // clear the set
    int max_fd = -1;
    for (const auto& pair : event_info_map) {
        const auto& info = pair.second;
        if (info.is_open) {
            FD_SET(info.fd, &read_fds);
            max_fd = (info.fd > max_fd) ? info.fd : max_fd;
        }
    }

    struct timeval timeout;                                                                    // set timeout
    timeout.tv_sec = time_out;
    timeout.tv_usec = 0;
    log_thread_safe(LOG_LEVEL_INFO, KEY_TAG, "Waiting for key press, timeout: %d seconds...", time_out);
    int ret = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);                               // wait for event or timeout
    if (ret == -1) {
        log_thread_safe(LOG_LEVEL_ERROR, KEY_TAG, "select error: %s", strerror(errno));
        return -1;
    } else if (ret == 0) {
        log_thread_safe(LOG_LEVEL_INFO, KEY_TAG, "Timeout, no key pressed within %d seconds", time_out);
        return 0;
    }

    for (const auto& pair : event_info_map) {                                                // check which device has event
        const auto& info = pair.second;
        if (info.is_open && FD_ISSET(info.fd, &read_fds)) {                                            // device has event
            struct input_event ev;
            ssize_t n = read(info.fd, &ev, sizeof(ev));                               // read event
            if (n == -1) {
                log_thread_safe(LOG_LEVEL_ERROR, KEY_TAG, "read event failed for device fd %d: %s", info.fd, strerror(errno));
                continue;
            }

            if (ev.type == EV_KEY) {                                                          // is key event
                if (ev.value == 1) {                                                          // key press event
                    log_thread_safe(LOG_LEVEL_INFO, KEY_TAG, "Detected key press - Key code: %d", ev.code);
                    return ev.code;
                }
            }
        }
    }
    return -1;
}

Key::~Key() {
    for (const auto& pair : event_info_map) {
        const auto& info = pair.second;
        if (info.is_open) {
            close(info.fd);
            log_thread_safe(LOG_LEVEL_INFO, KEY_TAG, "Closed device file descriptor for event: %s", pair.first.c_str());
        }
    }    
}
