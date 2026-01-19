#ifndef __AUDIO_H__
#define __AUDIO_H__

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <map>
#include <iostream>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <dirent.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <complex>
#include <valarray>
#include <vector>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <fftw3.h>
#include <iomanip>
#include <fstream>

#include "util/Log.h"
using namespace std;
// 音频参数常量已在类中定义，避免宏定义冲突
// #define CHANNELS 2
// #define SAMPLE_RATE 44100
// #define FORMAT SND_PCM_FORMAT_S16_LE
// #define BUFFER_SIZE 1024

#define DEV_INPUT "/dev/input"
#define EVENT_DEV "event"

class Audio {
public:
    enum ChannelMode {
        LEFT_ONLY,    // 仅左声道
        RIGHT_ONLY,   // 仅右声道
        STEREO        // 双声道
    };
       
    Audio(const char* audioEventName = "rockchip-es8389 Headset", const char* audioCardName = "plughw:rockchipes8388");
    
    // 音频播放相关函数
    void play_audio(int time);
    void play_audio(const char *device, short *buffer, snd_pcm_uframes_t frames);
    
    // 音频录制相关函数
    void recordAudio(int time);
    void recordAudio(int time, short* recorded_buffer);
    void recordAudio(int time, short* recorded_buffer, double volume_scale);
    
    // 音量控制函数
    void setRecordVolume(double volume_scale);
    double getRecordVolume() const;
    
    // 设备检测和选择
    bool findEventDevice();
    void selectAudioIn();
    
    // 音频分析和比较
    void recordAndCompare(int time);
    int record_and_compare_audio_v2(int time);
    
    // 工具函数
    void generate_tone(short *buffer, int frames);
    void save_buffer_to_wav(const char *filename, short *buffer, int frames);
    // 生成指定频率的双声道音频（左右声道不同频率）
    void generate_dual_tone(short *buffer, short *left_buffer, short *right_buffer, double left_freq, double right_freq, int frames);

private:
    const char* AUDIO_TAG = "Audio";
    const char* AUDIO_EVENT_NAME;
    const char* AUDIO_CARD_NAME;
    char dev_path[512];
    
    // 音频参数常量
    static const int CHANNELS = 2;
    static const int SAMPLE_RATE = 44100;
    static const snd_pcm_format_t FORMAT = SND_PCM_FORMAT_S16_LE;
    static const int BUFFER_SIZE = 1024;
    
    // 音量控制
    double record_volume_scale;
    
    // 内部函数
    void check_error(int err, const char *operation);

    // 自动增益控制函数
    double calculate_signal_rms(short *signal, int samples);
    double calculate_signal_peak(short *signal, int samples);
    double calculate_auto_gain(double play_rms, double record_rms, double play_peak, double record_peak);
    void apply_gain_for_buffer(short *buffer, int samples, double gain);

    bool read_wav_simple(const string &path,
                        vector<double> &samples,
                        uint32_t &sample_rate);

    void apply_hann(vector<double> &x);

    int detect_main_frequencies(const vector<double> &samples,
                                uint32_t sr, int top_k);
  
    double left_freq_min = 230;
    double left_freq     = 250;  
    double left_freq_max = 270; 

    double right_freq_min = 380; 
    double right_freq     = 400; 
    double right_freq_max = 420; 
};

#endif