#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <dirent.h>
#include <ctype.h>
#include <libudev.h>
#include <sys/stat.h>

pid_t child_pid = -1;
char target_device[32] = {0};
char mount_point[256] = {0};
const char *target_program = "SIGNWAY_CMBOARD_TESTAPP";
const char *copy_path = "/tmp/SIGNWAY_CMBOARD_TESTAPP";

// 清理函数
void cleanup_resources() {
    printf("Cleaning up resources...\n");
    
    if (child_pid > 0) {
        kill(child_pid, SIGKILL);
        waitpid(child_pid, NULL, 0);
        child_pid = -1;
    }
    
    // 清理挂载点
    if (strlen(mount_point) > 0) {
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "umount %s 2>/dev/null", mount_point);
        system(cmd);
        
        // 只删除我们创建的挂载点 (/mnt/ 目录下的)
        if (strncmp(mount_point, "/mnt/", 5) == 0) {
            snprintf(cmd, sizeof(cmd), "rmdir %s 2>/dev/null", mount_point);
            system(cmd);
        }
        
        mount_point[0] = '\0';
    }
    
    target_device[0] = '\0';
}

// 信号处理函数
// void signal_handler(int signum) {
//     if (signum == SIGUSR1) {
//         printf("Received SIGUSR1, terminating child process...\n");
//         cleanup_resources();
//     }
// }

// 检查是否为SD设备
int is_sd_device(const char *devname) {
    if (strncmp(devname, "sd", 2) != 0) {
        return 0;
    }
    
    // 检查是否是分区设备 (sda1, sdb2等) 或者整个磁盘 (sda, sdb等)
    size_t len = strlen(devname);
    if (len >= 3) {
        // 如果是整个磁盘设备 (sda, sdb)
        if (len == 3 && isalpha(devname[2]) && islower(devname[2])) {
            return 1;
        }
        // 如果是分区设备 (sda1, sdb2等)
        if (len > 3 && isalpha(devname[2]) && islower(devname[2]) && isdigit(devname[3])) {
            return 1;
        }
    }
    
    return 0;
}

// 获取设备挂载点 - 使用静态缓冲区
char* get_mount_point(const char *device, char *buffer, size_t buffer_size) {
    FILE *fp = fopen("/proc/mounts", "r");
    if (!fp) {
        return NULL;
    }
    
    char line[512];
    int found = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        char device_path[256], mount_path[256], fs_type[256];
        unsigned int options;
        
        if (sscanf(line, "%255s %255s %255s %u", device_path, mount_path, fs_type, &options) == 4) {
            // 检查完整的设备路径
            char full_device[256];
            snprintf(full_device, sizeof(full_device), "/dev/%s", device);
            if (strcmp(device_path, full_device) == 0) {
                strncpy(buffer, mount_path, buffer_size - 1);
                buffer[buffer_size - 1] = '\0';
                found = 1;
                break;
            }
        }
    }
    
    fclose(fp);
    return found ? buffer : NULL;
}

// 挂载设备
int mount_device(const char *device) {
    // 先清理之前的挂载
    if (strlen(mount_point) > 0) {
        cleanup_resources();
    }
    
    // 创建挂载点目录
    char mount_dir[256];
    snprintf(mount_dir, sizeof(mount_dir), "/mnt/%s", device);
    
    // 检查目录是否存在，不存在则创建
    if (access(mount_dir, F_OK) != 0) {
        if (mkdir(mount_dir, 0755) != 0) {
            perror("mkdir failed");
            return -1;
        }
    } else {
        // 如果目录已存在，确保它没有被挂载
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "umount %s 2>/dev/null", mount_dir);
        system(cmd);
    }
    
    // 挂载设备
    char mount_cmd[512];
    snprintf(mount_cmd, sizeof(mount_cmd), "mount /dev/%s %s", device, mount_dir);
    
    printf("Executing: %s\n", mount_cmd);
    if (system(mount_cmd) != 0) {
        printf("Mount failed for device %s\n", device);
        // 清理创建的目录
        char rmdir_cmd[256];
        snprintf(rmdir_cmd, sizeof(rmdir_cmd), "rmdir %s 2>/dev/null", mount_dir);
        system(rmdir_cmd);
        return -1;
    }
    
    strncpy(mount_point, mount_dir, sizeof(mount_point) - 1);
    return 0;
}

// 检测并启动程序
void detect_and_run_program(const char *device) {
    printf("Checking device: %s\n", device);
    
    // 如果已经有程序在运行，直接返回
    if (child_pid > 0) {
        printf("Program already running, skip device %s\n", device);
        return;
    }
    
    char current_mount_point[256] = {0};
    int mounted_by_us = 0;
    
    // 检查设备是否已经挂载
    if (!get_mount_point(device, current_mount_point, sizeof(current_mount_point))) {
        printf("Device %s not mounted, mounting...\n", device);
        if (mount_device(device) != 0) {
            return;
        }
        strcpy(current_mount_point, mount_point);
        mounted_by_us = 1;
    } else {
        printf("Device %s already mounted at %s\n", device, current_mount_point);
    }
    
    // 检查程序文件是否存在
    char program_path[512];
    snprintf(program_path, sizeof(program_path), "%s/%s", current_mount_point, target_program);
    
    printf("Looking for program at: %s\n", program_path);
    
    if (access(program_path, F_OK) == 0) {
        printf("Found target program: %s\n", program_path);
        
        // 拷贝程序
        char copy_cmd[512];
        snprintf(copy_cmd, sizeof(copy_cmd), "cp \"%s\" \"%s\"", program_path, copy_path);
        printf("Executing: %s\n", copy_cmd);
        
        if (system(copy_cmd) != 0) {
            printf("Copy failed\n");
            goto cleanup;
        }
        
        snprintf(copy_cmd, sizeof(copy_cmd), "chmod 755 \"%s\"", copy_path);
        system(copy_cmd);
        
        // 启动程序
        pid_t pid = fork();
        if (pid == 0) {
            // 子进程
            execl(copy_path, copy_path, (char*)NULL);
            perror("execl failed");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            // 父进程
            child_pid = pid;
            strncpy(target_device, device, sizeof(target_device) - 1);
            printf("Program started with PID: %d for device: %s\n", child_pid, device);
            // 不要清理挂载点，等设备移除时再清理
            return;
        } else {
            perror("fork failed");
            goto cleanup;
        }
    } else {
        printf("Target program not found in %s\n", program_path);
        // 列出目录内容以便调试
        char ls_cmd[512];
        snprintf(ls_cmd, sizeof(ls_cmd), "ls -la \"%s\"", current_mount_point);
        printf("Directory contents:\n");
        system(ls_cmd);
        goto cleanup;
    }

cleanup:
    // 如果是我们挂载的设备，需要卸载
    if (mounted_by_us) {
        cleanup_resources();
    }
}

// 扫描所有SD设备
void scan_sd_devices() {
    printf("Scanning for SD devices...\n");
    
    DIR *dir = opendir("/dev");
    if (!dir) {
        perror("opendir /dev failed");
        return;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (is_sd_device(entry->d_name)) {
            printf("Found SD device: %s\n", entry->d_name);
            detect_and_run_program(entry->d_name);
            
            // 如果已经启动了程序，就停止扫描
            if (child_pid > 0) {
                break;
            }
        }
    }
    
    closedir(dir);
}

// 处理udev设备事件
int handle_device(struct udev_device *dev) {
    const char *action = udev_device_get_action(dev);
    const char *devnode = udev_device_get_devnode(dev);
    const char *subsystem = udev_device_get_subsystem(dev);
    
    if (strcmp(subsystem, "block") != 0 || devnode == NULL) {
        return -1;
    }
    
    // 提取设备名（去掉/dev/前缀）
    const char *device_name = strrchr(devnode, '/');
    if (device_name) {
        device_name++; // 跳过 '/'
    } else {
        device_name = devnode;
    }
    
    if (!is_sd_device(device_name)) {
        return -1;
    }
    
    printf("USB device event: %s - %s\n", action, device_name);
    
    if (strcmp(action, "add") == 0) {
        // 如果是目标设备被移除后重新插入，或者还没有程序在运行
        if (child_pid <= 0) {
            detect_and_run_program(device_name);
        }
    } else if (strcmp(action, "remove") == 0) {
        // 检查是否是运行程序的设备被移除
        if (strlen(target_device) > 0 && strcmp(target_device, device_name) == 0) {
            printf("Target USB device removed: %s\n", device_name);
            kill(getpid(), SIGUSR1);
        }
    }
    
    return 0;
}

int main(int argc, char *argv[]) {
    printf("USB Auto-Run Manager started\n");
    
    // 注册信号处理函数
    // signal(SIGUSR1, signal_handler);
    
    // 确保程序退出时清理资源
    atexit(cleanup_resources);
    
    // 开机时扫描所有SD设备
    scan_sd_devices();
    
    // 设置udev监控
    struct udev *udev = udev_new();
    if (!udev) {
        fprintf(stderr, "Can't create udev\n");
        return -1;
    }
    
    struct udev_monitor *mon = udev_monitor_new_from_netlink(udev, "udev");
    if (!mon) {
        fprintf(stderr, "Can't create udev monitor\n");
        udev_unref(udev);
        return -1;
    }
    
    udev_monitor_filter_add_match_subsystem_devtype(mon, "block", NULL);
    udev_monitor_enable_receiving(mon);
    int fd = udev_monitor_get_fd(mon);
    
    printf("Starting udev monitoring...\n");
    
    while (1) {
        fd_set fds;
        struct timeval tv;
        int ret;
        
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        
        ret = select(fd + 1, &fds, NULL, NULL, &tv);
        
        if (ret > 0 && FD_ISSET(fd, &fds)) {
            struct udev_device *dev = udev_monitor_receive_device(mon);
            if (dev) {
                handle_device(dev);
                udev_device_unref(dev);
            }
        }
        
        // 检查子进程状态（非阻塞）
        if (child_pid > 0) {
            int status;
            pid_t result = waitpid(child_pid, &status, WNOHANG);
            if (result > 0) {
                printf("Child process %d exited\n", child_pid);
                child_pid = -1;
                target_device[0] = '\0';
            }
        }
        
        // 检查挂载点是否仍然有效
        if (strlen(mount_point) > 0 && child_pid <= 0) {
            char check_cmd[512];
            snprintf(check_cmd, sizeof(check_cmd), "mountpoint -q %s", mount_point);
            if (system(check_cmd) != 0) {
                printf("Mount point %s is no longer valid, cleaning up\n", mount_point);
                mount_point[0] = '\0';
            }
        }
    }
    
    udev_unref(udev);
    return 0;
}