# TestApp
```shell
    * test app 源码
    * main 函数                                   ： ./TestApp/src/main.cpp
    * 各个功能测试程序                             ： ./TestApp/src/hardware 
    * 任务分类，测试功能                           ： ./TestApp/src/task/TaskHandler.cpp
    * 协议解析：                                  :  ./TestApp/src/protocol/ProtocolParser.cpp
    * json解析工具，线程池，日志打印等功能          ： ./TestApp/src/util
```

# TestAppBootUp
```shell
    * 负责启动 test app 的程序 ./TestAppBootUp/test_app_start_v2.c 需要编译预制到固件中开机自启 ： 未曾有更新
    * 负责启动test app程序的程序，此程序需要开机自启
    * TestAppBootUp程序开机启动后会首先检测u盘内是否有特定名称的文件(该文件必须是可执行文件)。
    * 如果有则会将可执行文件拷贝至系统/tmp目录下，随后运行。拔出u盘结束运行。此后可热插拔u盘，启动或结束程序运行
    * TestAppBootUp 开机自启方式

# 1. 首先由/etc/systemd/system/signway.service服务启动signway.sh  
[Unit]
Description=Run My Script at Startup
After=default.target

[Service]
Type=forking                                # Tpye 必须为 forking 否则 signway.sh 中无法后台运行另一个程序
ExecStart=/etc/init.d/signway.sh
WorkingDirectory=/etc/init.d/
Restart=no
StandardOutput=journal
StandardError=journal
#Environment="VAR1=value1" "VAR2=value2"

[Install]
WantedBy=default.target


# 2. /etc/init.d/signway.sh 再去启动TestAppBootUp 程序
export DISPLAY=:0                                                                                                                  # 指定默认显示器
export XAUTHORITY=/home/ubuntu/.Xauthority                                                                                         # 程序中弹框需要该ui
export DBUS_SESSION_BUS_ADDRESS=$(grep -z DBUS_SESSION_BUS_ADDRESS /proc/$(pgrep -u ubuntu gnome-session)/environ | cut -d= -f2-)  # 设置系统永不休眠
sudo -u ubuntu gsettings set org.gnome.desktop.session idle-delay 0                                                                # 设置系统永不休眠 
touch /var/log/test_app.log                                                                                                        # 创建日志文件       
echo APP_START >  /var/log/test_app.log                                                                                            # 
/etc/init.d/test_app_start >> /var/log/test_app.log 2>&1 &                                                                         # 重定向test app日志输出至 /var/log/test_app.log 
```

# 依赖的库和软件
```shell
apt-get -y install make                  # 构建工具
apt-get -y install libudev*              # TestAppBootUp（负责启动test app的程序）。需要使用libudev-dev libusb-1.0-0-dev 库中相关接口，监听u盘拔插          
apt-get -y install libusb-1.0-0-dev      # TestAppBootUp（负责启动test app的程序）。需要使用libudev-dev libusb-1.0-0-dev 库中相关接口，监听u盘拔插  
apt-get -y install libbluetooth*         # test app 蓝牙测试方案中，需要使用libbluetooth ... 等库中相关接口，扫描蓝牙信息。
apt-get -y install libasound2-dev        # test app 喇叭,麦克风测试方案中， 需要使用 libasound2-dev 库中相关接口进行录制声音，播放声音。
apt-get -y install ffmpeg                # test app 摄像头测试方案中，摄像头抓取图片为 uyvy格式。 通过ffmpeg软件将uyvy转换为rgb格式后ubuntu系统方可预览。
apt-get -y install libpng-dev            # test app 摄像头测试方案中，摄像头抓取图片为 uyvy格式。 通过ffmpeg软件将uyvy转换为rgb格式后ubuntu系统方可预览。libpng-dev 库中接口可读取图像相关信息。
apt-get -y install libfftw3-dev          # test app 音频相关测试方案中, 需要使用libfftw3-dev库中相关接口，  对应音频数据进行傅里叶变换，时域转频域，查看录制到的频率信息并进行对比。
apt-get -y install evtest                # ubuntu 中查看按键上抛键值工具，可初步查看，测试 /dev/input 事件相关信息
apt-get -y install cutecom               # ubuntu 下的串口工具，请使用sudo 权限打开
apt-get -y install audacity              # ubutnu 下的音频分析软件
```
# 提交记录
## 2026-01-19
```shell
commit 565c2dda9c9ef512fa79cb4a5ad388573668a7e2 (HEAD -> main, origin/main, origin/HEAD)
Author: fjl <jl.f@signway.cn>
Date:   Mon Jan 19 14:37:18 2026 +0800

    feat:Optimize the test program and log printing, and add the thread pool function.Not fully optimized
```

