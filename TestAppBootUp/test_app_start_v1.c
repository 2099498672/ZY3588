#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libudev.h>

pid_t child_pid = -1;
char target_device[32] = {0};
char copy_path[256] = {0};

// 清理资源
void cleanup_resources() {
    printf("Cleaning up resources...\n");
    
    // 终止子进程
    if (child_pid > 0) {
        kill(child_pid, SIGKILL);
        waitpid(child_pid, NULL, 0);
        printf("Child process %d terminated\n", child_pid);
        child_pid = -1;
    }
    
    // 删除拷贝的程序
    if (strlen(copy_path) > 0) {
        remove(copy_path);
        printf("Removed copied program: %s\n", copy_path);
        copy_path[0] = '\0';
    }
    
    target_device[0] = '\0';
}

// 信号处理
void signal_handler(int signum) {
    if (signum == SIGUSR1) {
        printf("Target device removed, cleaning up...\n");
        cleanup_resources();
    }
}

// 检查是否为SD设备
int is_sd_device(const char *devname) {
    return (strncmp(devname, "sd", 2) == 0);
}

// 获取设备挂载点
char* get_mount_point(const char *device, char *buffer, size_t buffer_size) {
    FILE *fp = fopen("/proc/mounts", "r");
    if (!fp) return NULL;
    
    char line[512];
    char full_device[256];
    snprintf(full_device, sizeof(full_device), "/dev/%s", device);
    
    while (fgets(line, sizeof(line), fp)) {
        char device_path[256], mount_path[256];
        if (sscanf(line, "%255s %255s", device_path, mount_path) == 2) {
            if (strcmp(device_path, full_device) == 0) {
                // 检查是否在/media/ubuntu/目录下
                if (strncmp(mount_path, "/media/ubuntu/", 14) == 0) {
                    strncpy(buffer, mount_path, buffer_size - 1);
                    buffer[buffer_size - 1] = '\0';
                    fclose(fp);
                    return buffer;
                }
            }
        }
    }
    
    fclose(fp);
    return NULL;
}

// 检测并运行程序
void detect_and_run_program(const char *device) {
    printf("Checking device: /dev/%s\n", device);
    
    // 如果已有程序运行，跳过
    if (child_pid > 0) {
        printf("Program already running, skip device /dev/%s\n", device);
        return;
    }
    
    // 获取挂载点
    char mount_point[256];
    if (!get_mount_point(device, mount_point, sizeof(mount_point))) {
        printf("Device /dev/%s not mounted at /media/ubuntu/\n", device);
        return;
    }
    
    printf("Device mounted at: %s\n", mount_point);
    
    // 查找以CMBOARD_TESTAPP_开头的程序
    DIR *dir = opendir(mount_point);
    if (!dir) {
        perror("opendir failed");
        return;
    }
    
    struct dirent *entry;
    const char *TARGET_PREFIX = "CMBOARD_TESTAPP_";
    size_t prefix_len = strlen(TARGET_PREFIX);
    char found_program[512] = {0};
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
            
        if (strncmp(entry->d_name, TARGET_PREFIX, prefix_len) == 0) {
            snprintf(found_program, sizeof(found_program), "%s/%s", mount_point, entry->d_name);
            printf("Found target program: %s\n", found_program);
            break;
        }
    }
    closedir(dir);
    
    if (strlen(found_program) == 0) {
        printf("No target program found in %s\n", mount_point);
        return;
    }
    
    // 拷贝到home目录
    const char *filename = strrchr(found_program, '/');
    filename = (filename ? filename + 1 : found_program);
    snprintf(copy_path, sizeof(copy_path), "/home/ubuntu/%s", filename);
    
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "cp \"%s\" \"%s\"", found_program, copy_path);
    printf("Copying: %s\n", cmd);
    
    if (system(cmd) != 0) {
        printf("Copy failed\n");
        return;
    }
    
    // 设置权限
    snprintf(cmd, sizeof(cmd), "chmod 755 \"%s\"", copy_path);
    system(cmd);
    
    // 启动程序
    pid_t pid = fork();
    if (pid == 0) {
        // 子进程
        execl(copy_path, copy_path, (char *)NULL);
        perror("execl failed");
        exit(1);
    } else if (pid > 0) {
        // 父进程
        child_pid = pid;
        strncpy(target_device, device, sizeof(target_device) - 1);
        printf("Program started: PID=%d, Device=%s\n", child_pid, target_device);
    } else {
        perror("fork failed");
    }
}

// 扫描已挂载的SD设备
void scan_mounted_devices() {
    printf("Scanning mounted SD devices in /media/ubuntu/...\n");
    
    DIR *dir = opendir("/dev");
    if (!dir) {
        perror("opendir /dev failed");
        return;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (is_sd_device(entry->d_name)) {
            detect_and_run_program(entry->d_name);
            if (child_pid > 0) break; // 找到一个就停止
        }
    }
    
    closedir(dir);
}

// 处理udev事件
int handle_device(struct udev_device *dev) {
    const char *action = udev_device_get_action(dev);
    const char *devnode = udev_device_get_devnode(dev);
    const char *subsystem = udev_device_get_subsystem(dev);
    
    if (strcmp(subsystem, "block") != 0 || !devnode) 
        return -1;
    
    const char *device_name = strrchr(devnode, '/');
    device_name = device_name ? device_name + 1 : devnode;
    
    if (!is_sd_device(device_name))
        return -1;
    
    printf("UDEV event: %s - %s\n", action, devnode);
    
    if (strcmp(action, "add") == 0) {
        // 等待设备挂载
        sleep(2);
        if (child_pid <= 0) {
            detect_and_run_program(device_name);
        }
    } else if (strcmp(action, "remove") == 0) {
        if (strlen(target_device) > 0 && strcmp(target_device, device_name) == 0) {
            printf("Target device removed, cleaning up\n");
            kill(getpid(), SIGUSR1);
        }
    }
    
    return 0;
}

int main() {
    printf("=========================================\n");
    printf("USB Auto-Run Manager\n");
    printf("Target: CMBOARD_TESTAPP_* in /media/ubuntu/\n");
    printf("=========================================\n");
    
    signal(SIGUSR1, signal_handler);
    atexit(cleanup_resources);
    
    // 首次扫描
    scan_mounted_devices();
    
    // udev监控
    struct udev *udev = udev_new();
    if (!udev) {
        fprintf(stderr, "Failed to create udev\n");
        return -1;
    }
    
    struct udev_monitor *mon = udev_monitor_new_from_netlink(udev, "udev");
    udev_monitor_filter_add_match_subsystem_devtype(mon, "block", NULL);
    udev_monitor_enable_receiving(mon);
    int udev_fd = udev_monitor_get_fd(mon);
    
    printf("Monitoring USB devices...\n");
    
    while (1) {
        fd_set fds;
        struct timeval tv = {5, 0}; // 5秒超时
        
        FD_ZERO(&fds);
        FD_SET(udev_fd, &fds);
        
        int ret = select(udev_fd + 1, &fds, NULL, NULL, &tv);
        
        if (ret > 0 && FD_ISSET(udev_fd, &fds)) {
            struct udev_device *dev = udev_monitor_receive_device(mon);
            if (dev) {
                handle_device(dev);
                udev_device_unref(dev);
            }
        }
        
        // 检查子进程状态
        if (child_pid > 0) {
            int status;
            if (waitpid(child_pid, &status, WNOHANG) > 0) {
                printf("Child process exited\n");
                cleanup_resources();
            }
        }
    }
    
    udev_unref(udev);
    return 0;
}