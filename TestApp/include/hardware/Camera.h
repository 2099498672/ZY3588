#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <png.h>
#include <linux/videodev2.h>
#include <map>

#include "util/Log.h"

class Camera {
private:
    const char* CAMERA_TAG = "Camera";

    // V4L2摄像头捕获相关私有方法
    bool initV4L2Device(int fd, unsigned char* buffers[], size_t buffer_lengths[], int buffer_num);
    bool startV4L2Capturing(int fd, int buffer_num);
    bool captureV4L2Frame(int fd, unsigned char* buffers[], size_t buffer_lengths[], int buffer_num, const char* outputFile);
    void stopV4L2Capturing(int fd);
    void uninitV4L2Device(unsigned char* buffers[], size_t buffer_lengths[], int buffer_num);
    bool saveFrameAsPng(const void* data, size_t size, const char* outputFile);
    void showPicture(const char* filePath);
public:
    Camera();

    /*
     * @brief 摄像头测试, 直接使用命令拍照，保存为png文件，解析png文件获取分辨率信息
     */
    virtual bool cameraTestCam0(std::string& picturePixel);
    virtual bool cameraTestCam1(std::string& picturePixel);


    struct CameraInfo {
        std::string cameraId;
        std::string devicePath;
        std::string outputFilePath;
    };
    std::map<std::string, CameraInfo> cameraidToInfo;

    virtual bool cameraCapturePng(const char* device, const char* outputFile);
};

#endif // __CAMERA_H__

