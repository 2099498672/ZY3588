#include "hardware/Camera.h"
#include <fcntl.h>
#include <errno.h>
#include <string.h>

Camera::Camera() {

}

bool Camera::cameraTestCam0(std::string& picturePixel) {
    const char *command = "v4l2-ctl --device=/dev/video22 --set-fmt-video=width=1920,height=1080,pixelformat=UYVY --stream-mmap --stream-count=1 --stream-to=- | ffmpeg -y -f rawvideo -pixel_format uyvy422 -video_size 1920x1080 -i - -frames 1 test.png";
    LogInfo(CAMERA_TAG, "Executing command: %s", command);

    int ret = system(command);
    if (ret != 0) {
        LogError(CAMERA_TAG, "Camera test failed with return code %d", ret);
        return false;
    }

    FILE *fp = fopen("test.png", "r");
    if (fp == NULL) {
        LogError(CAMERA_TAG, "Failed to open test.png");
        return false;
    }

    png_byte sig[8];
    fread(sig, 1, 8, fp);
    if (!png_check_sig(sig, 8)) {
        fclose(fp);
        fprintf(stderr, "不是有效的PNG文件\n");
        return false;
    }

    // 初始化PNG结构
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fclose(fp);
        return -1;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(fp);
        return -1;
    }

    // 错误处理
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return -1;
    }

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);
    png_read_info(png_ptr, info_ptr);

    // 获取图像信息
    png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
    png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
    png_byte color_type = png_get_color_type(png_ptr, info_ptr);
    png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    printf("图像尺寸: %dx%d\n", width, height);
    printf("颜色类型: %d, 位深度: %d\n", color_type, bit_depth);

    picturePixel = std::to_string(width) + "x" + std::to_string(height);
    
    const char *rmCmd = "rm -f test.png";
    ret = system(rmCmd);
    if (ret != 0) {
        LogError(CAMERA_TAG, "Failed to remove test.png");
        return false;
    }
  
    return true;
}

bool Camera::cameraTestCam1(std::string& picturePixel) {
    const char *command = "v4l2-ctl --device=/dev/video31 --set-fmt-video=width=1920,height=1080,pixelformat=UYVY --stream-mmap --stream-count=1 --stream-to=- | ffmpeg -y -f rawvideo -pixel_format uyvy422 -video_size 1920x1080 -i - -frames 1 test.png";
    LogInfo(CAMERA_TAG, "Executing command: %s", command);
    int ret = system(command);
    if (ret != 0) {
        LogError(CAMERA_TAG, "Camera test failed with return code %d", ret);
        return false;
    }

    FILE *fp = fopen("test.png", "r");
    if (fp == NULL) {
        LogError(CAMERA_TAG, "Failed to open test.png");
        return false;
    }

    png_byte sig[8];
    fread(sig, 1, 8, fp);
    if (!png_check_sig(sig, 8)) {
        fclose(fp);
        fprintf(stderr, "不是有效的PNG文件\n");
        return false;
    }

    // 初始化PNG结构
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fclose(fp);
        return -1;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(fp);
        return -1;
    }

    // 错误处理
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return -1;
    }

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);
    png_read_info(png_ptr, info_ptr);

    // 获取图像信息
    png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
    png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
    png_byte color_type = png_get_color_type(png_ptr, info_ptr);
    png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    printf("图像尺寸: %dx%d\n", width, height);
    printf("颜色类型: %d, 位深度: %d\n", color_type, bit_depth);

    picturePixel = std::to_string(width) + "x" + std::to_string(height);
    
    const char *rmCmd = "rm -f test.png";
    ret = system(rmCmd);
    if (ret != 0) {
        LogError(CAMERA_TAG, "Failed to remove test.png");
        return false;
    }
  
    return true;
}



bool Camera::cameraCapturePng(const char* device, const char* outputFile) {
    LogInfo(CAMERA_TAG, "开始从设备 %s 捕获PNG图像到 %s", device, outputFile);

    int fd = -1;
    const int BUFFER_NUM = 1;
    unsigned char* buffers[BUFFER_NUM] = { NULL };
    size_t buffer_lengths[BUFFER_NUM] = { 0 };
    bool success = false;

    // 打开设备
    fd = open(device, O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        LogError(CAMERA_TAG, "打开设备失败: %s, 错误: %s", device, strerror(errno));
        return false;
    }

    // 初始化设备
    if (!initV4L2Device(fd, buffers, buffer_lengths, BUFFER_NUM)) {
        LogError(CAMERA_TAG, "初始化V4L2设备失败");
        close(fd);
        return false;
    }

    // 开始捕获
    if (!startV4L2Capturing(fd, BUFFER_NUM)) {
        LogError(CAMERA_TAG, "开始捕获失败");
        uninitV4L2Device(buffers, buffer_lengths, BUFFER_NUM);
        close(fd);
        return false;
    }

    // 捕获一帧
    success = captureV4L2Frame(fd, buffers, buffer_lengths, BUFFER_NUM, outputFile);

    // 停止捕获并清理资源
    stopV4L2Capturing(fd);
    uninitV4L2Device(buffers, buffer_lengths, BUFFER_NUM);
    close(fd);

    if (success) {
        LogInfo(CAMERA_TAG, "成功捕获PNG图像到: %s", outputFile);
    } else {
        LogError(CAMERA_TAG, "捕获PNG图像失败");
    }

    return success;
}

bool Camera::initV4L2Device(int fd, unsigned char* buffers[], size_t buffer_lengths[], int buffer_num) {
    int ret, i;
    struct v4l2_capability cap;
    struct v4l2_format fmt;
    struct v4l2_requestbuffers reqbufs;
    struct v4l2_buffer buf;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];

    // 查询设备能力
    memset(&cap, 0, sizeof(cap));
    ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
    if (ret < 0) {
        LogError(CAMERA_TAG, "VIDIOC_QUERYCAP失败: %s", strerror(errno));
        return false;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)) {
        LogError(CAMERA_TAG, "设备不支持多平面视频捕获");
        return false;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        LogError(CAMERA_TAG, "设备不支持流式I/O");
        return false;
    }

    LogInfo(CAMERA_TAG, "设备驱动: %s", cap.driver);
    LogInfo(CAMERA_TAG, "设备卡: %s", cap.card);
    LogInfo(CAMERA_TAG, "设备总线信息: %s", cap.bus_info);

    // 设置视频格式
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix_mp.width = 1920;
    fmt.fmt.pix_mp.height = 1080;
    fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_UYVY;
    fmt.fmt.pix_mp.field = V4L2_FIELD_NONE;
    fmt.fmt.pix_mp.num_planes = 1;
    fmt.fmt.pix_mp.plane_fmt[0].bytesperline = 1920;
    fmt.fmt.pix_mp.plane_fmt[0].sizeimage = 1920 * 1080 * 2; // UYVY格式为2字节/像素

    ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
    if (ret < 0) {
        LogError(CAMERA_TAG, "VIDIOC_S_FMT失败: %s", strerror(errno));
        return false;
    }

    LogInfo(CAMERA_TAG, "设置格式成功: %dx%d, 像素格式: 0x%08X",
            fmt.fmt.pix_mp.width, fmt.fmt.pix_mp.height,
            fmt.fmt.pix_mp.pixelformat);

    // 请求缓冲区
    memset(&reqbufs, 0, sizeof(reqbufs));
    reqbufs.count = buffer_num;
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    reqbufs.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(fd, VIDIOC_REQBUFS, &reqbufs);
    if (ret < 0) {
        LogError(CAMERA_TAG, "VIDIOC_REQBUFS失败: %s", strerror(errno));
        return false;
    }

    if (reqbufs.count < (unsigned int)buffer_num) {
        LogError(CAMERA_TAG, "缓冲区数量不足: %d (请求 %d)", reqbufs.count, buffer_num);
        return false;
    }

    LogInfo(CAMERA_TAG, "分配了 %d 个缓冲区", reqbufs.count);

    // 映射缓冲区
    for (i = 0; i < buffer_num; i++) {
        memset(&buf, 0, sizeof(buf));
        memset(planes, 0, sizeof(planes));
        buf.index = i;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.length = 1;
        buf.m.planes = planes;

        ret = ioctl(fd, VIDIOC_QUERYBUF, &buf);
        if (ret < 0) {
            LogError(CAMERA_TAG, "VIDIOC_QUERYBUF失败: %s", strerror(errno));
            return false;
        }

        buffer_lengths[i] = buf.m.planes[0].length;
        buffers[i] = (unsigned char*)mmap(NULL, buffer_lengths[i], PROT_READ | PROT_WRITE,
            MAP_SHARED, fd, buf.m.planes[0].m.mem_offset);

        if (buffers[i] == MAP_FAILED) {
            LogError(CAMERA_TAG, "缓冲区映射失败: %s", strerror(errno));
            return false;
        }

        LogInfo(CAMERA_TAG, "缓冲区[%d]: 长度=%zu, 偏移=%d",
                i, buffer_lengths[i], buf.m.planes[0].m.mem_offset);
    }

    return true;
}

bool Camera::startV4L2Capturing(int fd, int buffer_num) {
    int ret, i;
    struct v4l2_buffer buf;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];

    // 将所有缓冲区加入队列
    for (i = 0; i < buffer_num; i++) {
        memset(&buf, 0, sizeof(buf));
        memset(planes, 0, sizeof(planes));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        buf.length = 1;
        buf.m.planes = planes;

        ret = ioctl(fd, VIDIOC_QBUF, &buf);
        if (ret < 0) {
            LogError(CAMERA_TAG, "VIDIOC_QBUF失败: %s", strerror(errno));
            return false;
        }
    }

    // 开始流
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    ret = ioctl(fd, VIDIOC_STREAMON, &type);
    if (ret < 0) {
        LogError(CAMERA_TAG, "VIDIOC_STREAMON失败: %s", strerror(errno));
        return false;
    }

    LogInfo(CAMERA_TAG, "开始视频流捕获");
    return true;
}

bool Camera::captureV4L2Frame(int fd, unsigned char* buffers[], size_t buffer_lengths[], int buffer_num, const char* outputFile) {
    int ret;
    fd_set fds;
    struct timeval tv;
    struct v4l2_buffer buf;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    // 设置超时
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    // 等待数据可读
    ret = select(fd + 1, &fds, NULL, NULL, &tv);
    if (ret == -1) {
        if (errno == EINTR) {
            LogInfo(CAMERA_TAG, "select被信号中断，继续等待");
        } else {
            LogError(CAMERA_TAG, "select失败: %s", strerror(errno));
            return false;
        }
    }

    if (ret == 0) {
        LogError(CAMERA_TAG, "等待帧数据超时");
        return false;
    }

    // 从队列中取出缓冲区
    memset(&buf, 0, sizeof(buf));
    memset(planes, 0, sizeof(planes));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.length = 1;
    buf.m.planes = planes;

    ret = ioctl(fd, VIDIOC_DQBUF, &buf);
    if (ret < 0) {
        LogError(CAMERA_TAG, "VIDIOC_DQBUF失败: %s", strerror(errno));
        return false;
    }

    // 处理捕获的帧
    if (buf.m.planes[0].bytesused > 0) {
        LogInfo(CAMERA_TAG, "捕获到帧, 大小: %u 字节", buf.m.planes[0].bytesused);

        // 保存帧数据到PNG文件
        if (saveFrameAsPng(buffers[buf.index], buf.m.planes[0].bytesused, outputFile)) {
            LogInfo(CAMERA_TAG, "帧已保存到 %s", outputFile);
        } else {
            LogError(CAMERA_TAG, "保存帧失败");
            // 将缓冲区重新加入队列后返回
            ioctl(fd, VIDIOC_QBUF, &buf);
            return false;
        }
    }

    // 将缓冲区重新加入队列
    ret = ioctl(fd, VIDIOC_QBUF, &buf);
    if (ret < 0) {
        LogError(CAMERA_TAG, "VIDIOC_QBUF失败: %s", strerror(errno));
        return false;
    }

    // showPicture(outputFile);

    return true;
}

void Camera::showPicture(const char* filePath) {
    char command[256];
    snprintf(command, sizeof(command), "ffmpeg -loop 1 -i %s -t 3 -f sdl \"%s\"", filePath, filePath);
    system(command);
}

void Camera::stopV4L2Capturing(int fd) {
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0) {
        LogError(CAMERA_TAG, "VIDIOC_STREAMOFF失败: %s", strerror(errno));
    } else {
        LogInfo(CAMERA_TAG, "停止视频流捕获");
    }
}

void Camera::uninitV4L2Device(unsigned char* buffers[], size_t buffer_lengths[], int buffer_num) {
    int i;

    // 取消映射所有缓冲区
    for (i = 0; i < buffer_num; i++) {
        if (buffers[i] != NULL && buffers[i] != MAP_FAILED) {
            munmap(buffers[i], buffer_lengths[i]);
            buffers[i] = NULL;
        }
    }

    LogInfo(CAMERA_TAG, "释放所有缓冲区");
}

bool Camera::saveFrameAsPng(const void* data, size_t size, const char* outputFile) {
    // 使用系统命令将原始UYVY数据转换为PNG
    char command[512];
    snprintf(command, sizeof(command),
             "ffmpeg -y -f rawvideo -pixel_format uyvy422 -video_size 1920x1080 -i - -frames 1 %s",
             outputFile);
    
    LogInfo(CAMERA_TAG, "执行转换命令: %s", command);
    
    FILE* ffmpeg_pipe = popen(command, "w");
    if (ffmpeg_pipe == NULL) {
        LogError(CAMERA_TAG, "无法启动ffmpeg进程");
        return false;
    }

    size_t written = fwrite(data, 1, size, ffmpeg_pipe);
    if (written != size) {
        LogError(CAMERA_TAG, "写入ffmpeg管道失败");
        pclose(ffmpeg_pipe);
        return false;
    }

    int ret = pclose(ffmpeg_pipe);
    if (ret != 0) {
        LogError(CAMERA_TAG, "ffmpeg转换失败，返回码: %d", ret);
        return false;
    }

    return true;
}