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
// 修改：从const字符串改为可动态赋值的数组，存储拷贝后的程序路径
char copy_path[256] = {0};

// 清理函数
void cleanup_resources() {
    printf("Cleaning up resources...\n");
    
    // 1. 终止并回收子进程
    if (child_pid > 0) {
        kill(child_pid, SIGKILL);
        waitpid(child_pid, NULL, 0);
        printf("Child process %d terminated\n", child_pid);
        child_pid = -1;
    }
    
    // 2. 清理/tmp下拷贝的程序文件（新增）
    if (strlen(copy_path) > 0) {
        if (remove(copy_path) == 0) {
            printf("Cleaned up copied program: %s\n", copy_path);
        } else {
            perror("Failed to clean up copied program");
        }
        copy_path[0] = '\0';
    }
    
    // 3. 卸载并删除挂载点
    if (strlen(mount_point) > 0) {
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "umount %s 2>/dev/null", mount_point);
        system(cmd);
        
        // 只删除本程序创建的/mnt下的挂载点
        if (strncmp(mount_point, "/mnt/", 5) == 0) {
            snprintf(cmd, sizeof(cmd), "rmdir %s 2>/dev/null", mount_point);
            system(cmd);
            printf("Mount point %s removed\n", mount_point);
        }
        
        mount_point[0] = '\0';
    }
    
    target_device[0] = '\0';
}

// 信号处理函数（恢复注释，处理设备移除事件）
void signal_handler(int signum) {
    if (signum == SIGUSR1) {
        printf("\nReceived SIGUSR1 (target device removed), cleaning up...\n");
        cleanup_resources();
    }
}

// 检查是否为SD设备（逻辑不变）
int is_sd_device(const char *devname) {
    if (strncmp(devname, "sd", 2) != 0) {
        return 0;
    }
    
    size_t len = strlen(devname);
    // 匹配SD磁盘（sda/sdb）或分区（sda1/sdb2）
    if (len >= 3) {
        if ((len == 3 && isalpha(devname[2]) && islower(devname[2])) ||
            (len > 3 && isalpha(devname[2]) && islower(devname[2]) && isdigit(devname[3]))) {
            return 1;
        }
    }
    
    return 0;
}

// 获取设备挂载点（逻辑不变）
char* get_mount_point(const char *device, char *buffer, size_t buffer_size) {
    FILE *fp = fopen("/proc/mounts", "r");
    if (!fp) {
        perror("fopen /proc/mounts failed");
        return NULL;
    }
    
    char line[512];
    int found = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        char device_path[256], mount_path[256], fs_type[256];
        unsigned int options;
        
        if (sscanf(line, "%255s %255s %255s %u", device_path, mount_path, fs_type, &options) == 4) {
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

// 挂载设备（逻辑不变）
int mount_device(const char *device) {
    if (strlen(mount_point) > 0) {
        cleanup_resources();
    }
    
    // 创建挂载点（/mnt/[设备名]）
    char mount_dir[256];
    snprintf(mount_dir, sizeof(mount_dir), "/mnt/%s", device);
    
    if (access(mount_dir, F_OK) != 0) {
        if (mkdir(mount_dir, 0755) != 0) {
            perror("mkdir mount point failed");
            return -1;
        }
    } else {
        // 若目录已存在，先卸载
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "umount %s 2>/dev/null", mount_dir);
        system(cmd);
    }
    
    // 执行挂载
    char mount_cmd[512];
    snprintf(mount_cmd, sizeof(mount_cmd), "mount /dev/%s %s", device, mount_dir);
    printf("Executing mount command: %s\n", mount_cmd);
    
    if (system(mount_cmd) != 0) {
        printf("Mount failed for device /dev/%s\n", device);
        // 清理创建的空目录
        char rmdir_cmd[256];
        snprintf(rmdir_cmd, sizeof(rmdir_cmd), "rmdir %s 2>/dev/null", mount_dir);
        system(rmdir_cmd);
        return -1;
    }
    
    strncpy(mount_point, mount_dir, sizeof(mount_point) - 1);
    printf("Device /dev/%s mounted at %s\n", device, mount_point);
    return 0;
}

// 核心修改：检测并启动以"CMBOARD_TESTAPP_"开头的第一个程序
void detect_and_run_program(const char *device) {
    printf("\nChecking device: /dev/%s\n", device);
    
    // 若已有程序运行，跳过
    if (child_pid > 0) {
        printf("Program already running (PID: %d), skip device /dev/%s\n", child_pid, device);
        return;
    }
    
    char current_mount_point[256] = {0};
    int mounted_by_us = 0;
    
    // 检查设备是否已挂载
    if (!get_mount_point(device, current_mount_point, sizeof(current_mount_point))) {
        printf("Device /dev/%s not mounted, trying to mount...\n", device);
        if (mount_device(device) != 0) {
            printf("Mount failed, abort detection for /dev/%s\n", device);
            return;
        }
        strcpy(current_mount_point, mount_point);
        mounted_by_us = 1;
    } else {
        printf("Device /dev/%s already mounted at %s\n", device, current_mount_point);
    }
    
    // -------------------------- 核心修改：遍历目录寻找前缀匹配的程序 --------------------------
    DIR *dir = opendir(current_mount_point);
    if (!dir) {
        perror("opendir mount point failed");
        goto cleanup;
    }
    
    struct dirent *entry;
    char found_program_path[512] = {0}; // 存储找到的目标程序完整路径
    const char *TARGET_PREFIX = "CMBOARD_TESTAPP_"; // 目标程序前缀
    size_t prefix_len = strlen(TARGET_PREFIX);
    
    // 遍历目录项，寻找第一个符合条件的普通文件
    while ((entry = readdir(dir)) != NULL) {
        // 跳过 "." 和 ".." 目录
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // 检查文件名是否以目标前缀开头
        if (strlen(entry->d_name) < prefix_len || strncmp(entry->d_name, TARGET_PREFIX, prefix_len) != 0) {
            continue;
        }
        
        // 构造完整文件路径
        snprintf(found_program_path, sizeof(found_program_path), "%s/%s", current_mount_point, entry->d_name);
        
        // 验证是否为普通文件（避免执行目录/链接等）
        struct stat file_stat;
        if (stat(found_program_path, &file_stat) != 0) {
            perror("stat file failed (skip)");
            continue;
        }
        if (!S_ISREG(file_stat.st_mode)) {
            printf("Skipping non-regular file: %s\n", found_program_path);
            continue;
        }
        
        // 找到第一个符合条件的程序，退出遍历
        printf("Found target program: %s\n", found_program_path);
        break;
    }
    
    closedir(dir);
    // ------------------------------------------------------------------------------------------
    
    // 检查是否找到目标程序
    if (strlen(found_program_path) == 0) {
        printf("No program starting with \"%s\" found in %s\n", TARGET_PREFIX, current_mount_point);
        // 列出目录内容用于调试
        char ls_cmd[512];
        snprintf(ls_cmd, sizeof(ls_cmd), "ls -la \"%s\"", current_mount_point);
        printf("Directory contents:\n");
        system(ls_cmd);
        goto cleanup;
    }
    
    // 构造拷贝目标路径（/tmp/[找到的程序名]）
    const char *filename = strrchr(found_program_path, '/');
    if (filename == NULL) {
        filename = found_program_path; // 理论上不会触发（路径含挂载点）
    } else {
        filename++; // 跳过 '/'，获取纯文件名
    }
    snprintf(copy_path, sizeof(copy_path), "/tmp/%s", filename);
    
    // 拷贝程序到/tmp（避免设备占用问题）
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "cp \"%s\" \"%s\"", found_program_path, copy_path);
    printf("Executing copy command: %s\n", cmd);
    if (system(cmd) != 0) {
        printf("Copy program failed\n");
        goto cleanup;
    }
    
    // 赋予可执行权限
    snprintf(cmd, sizeof(cmd), "chmod 755 \"%s\"", copy_path);
    if (system(cmd) != 0) {
        perror("chmod executable failed");
        remove(copy_path); // 清理拷贝失败的文件
        goto cleanup;
    }
    
    // 启动程序（fork+execl）
    pid_t pid = fork();
    if (pid == 0) {
        // 子进程：执行程序
        execl(copy_path, copy_path, (char *)NULL);
        perror("execl failed to start program");
        remove(copy_path); // 执行失败，清理文件
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // 父进程：记录子进程信息
        child_pid = pid;
        strncpy(target_device, device, sizeof(target_device) - 1);
        printf("Program started successfully: PID=%d, Path=%s\n", child_pid, copy_path);
        return; // 不清理挂载点，设备移除时再处理
    } else {
        perror("fork failed");
        remove(copy_path); // fork失败，清理文件
        goto cleanup;
    }

cleanup:
    // 若为本程序挂载的设备，需卸载清理
    if (mounted_by_us) {
        printf("Cleaning up mount point for /dev/%s\n", device);
        cleanup_resources();
    }
    printf("Detection for /dev/%s completed\n", device);
}

// 扫描所有SD设备（逻辑不变）
void scan_sd_devices() {
    printf("\nScanning all SD devices in /dev...\n");
    
    DIR *dir = opendir("/dev");
    if (!dir) {
        perror("opendir /dev failed");
        return;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (is_sd_device(entry->d_name)) {
            detect_and_run_program(entry->d_name);
            // 找到一个程序并启动后，停止扫描
            if (child_pid > 0) {
                printf("Program started, stop SD device scan\n");
                break;
            }
        }
    }
    
    closedir(dir);
}

// 处理udev设备事件（逻辑不变，仅信号触发清理）
int handle_device(struct udev_device *dev) {
    const char *action = udev_device_get_action(dev);
    const char *devnode = udev_device_get_devnode(dev);
    const char *subsystem = udev_device_get_subsystem(dev);
    
    if (strcmp(subsystem, "block") != 0 || devnode == NULL) {
        return -1;
    }
    
    // 提取设备名（去掉/dev/前缀）
    const char *device_name = strrchr(devnode, '/');
    device_name = (device_name != NULL) ? (device_name + 1) : devnode;
    
    if (!is_sd_device(device_name)) {
        return -1;
    }
    
    printf("\nUDEV event detected: Action=%s, Device=%s (/dev/%s)\n", action, devnode, device_name);
    
    if (strcmp(action, "add") == 0) {
        // 设备插入：若无程序运行，检测并启动
        if (child_pid <= 0) {
            detect_and_run_program(device_name);
        }
    } else if (strcmp(action, "remove") == 0) {
        // 设备移除：若为当前运行程序的设备，触发清理
        if (strlen(target_device) > 0 && strcmp(target_device, device_name) == 0) {
            printf("Target device /dev/%s removed, sending SIGUSR1 to clean up\n", device_name);
            kill(getpid(), SIGUSR1);
        }
    }
    
    return 0;
}

int main(int argc, char *argv[]) {
    printf("=============================================\n");
    printf("USB Auto-Run Manager (Prefix: CMBOARD_TESTAPP_)\n");
    printf("=============================================\n");
    
    // 注册信号处理（设备移除时触发清理）
    signal(SIGUSR1, signal_handler);
    // 注册程序退出时的资源清理
    atexit(cleanup_resources);
    
    // 开机时扫描一次SD设备
    scan_sd_devices();
    
    // 初始化udev监控（监听块设备事件）
    struct udev *udev = udev_new();
    if (!udev) {
        fprintf(stderr, "Failed to create udev context\n");
        return -1;
    }
    
    struct udev_monitor *mon = udev_monitor_new_from_netlink(udev, "udev");
    if (!mon) {
        fprintf(stderr, "Failed to create udev monitor\n");
        udev_unref(udev);
        return -1;
    }
    
    // 过滤块设备事件
    udev_monitor_filter_add_match_subsystem_devtype(mon, "block", NULL);
    udev_monitor_enable_receiving(mon);
    int udev_fd = udev_monitor_get_fd(mon);
    
    printf("\nUDEV monitoring started (listening for block device events)...\n");
    printf("Press Ctrl+C to exit\n");
    
    while (1) {
        fd_set fds;
        struct timeval tv;
        int ret;
        
        FD_ZERO(&fds);
        FD_SET(udev_fd, &fds);
        tv.tv_sec = 5;    // 5秒超时，定期检查子进程状态
        tv.tv_usec = 0;
        
        // 等待udev事件或超时
        ret = select(udev_fd + 1, &fds, NULL, NULL, &tv);
        if (ret < 0) {
            perror("select failed");
            continue;
        }
        
        // 处理udev事件
        if (ret > 0 && FD_ISSET(udev_fd, &fds)) {
            struct udev_device *dev = udev_monitor_receive_device(mon);
            if (dev) {
                handle_device(dev);
                udev_device_unref(dev);
            }
        }
        
        // 检查子进程状态（非阻塞）
        if (child_pid > 0) {
            int status;
            pid_t wait_ret = waitpid(child_pid, &status, WNOHANG);
            if (wait_ret > 0) {
                // 子进程退出
                printf("\nChild process %d exited (status: 0x%x)\n", child_pid, status);
                cleanup_resources(); // 清理资源，准备接收新设备
            }
        }
        
        // 检查挂载点有效性（若无程序运行，清理无效挂载点）
        if (strlen(mount_point) > 0 && child_pid <= 0) {
            char check_cmd[512];
            snprintf(check_cmd, sizeof(check_cmd), "mountpoint -q %s", mount_point);
            if (system(check_cmd) != 0) {
                printf("Mount point %s is invalid, cleaning up\n", mount_point);
                cleanup_resources();
            }
        }
    }
    
    // 理论上不会执行到此处（循环永不退出）
    udev_unref(udev);
    return 0;
}