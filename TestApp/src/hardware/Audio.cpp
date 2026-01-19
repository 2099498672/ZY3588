#include "hardware/Audio.h"
#include <mutex>
#include <condition_variable>
#include <complex>
#include <valarray>
#include <chrono>
#include <vector>
#include <map>

Audio::Audio(const char* audioEventName, const char* AUDIO_CARD_NAME)
    :AUDIO_EVENT_NAME(audioEventName), 
    AUDIO_CARD_NAME(AUDIO_CARD_NAME) {

    record_volume_scale = 1.0; // 初始值为1.0，将由自动增益控制调整
}

// 错误处理函数
void Audio::check_error(int err, const char *operation) {
    if (err < 0) {
        log_thread_safe(LOG_LEVEL_ERROR, AUDIO_TAG, "%s error: %s", operation, snd_strerror(err));
        exit(EXIT_FAILURE);
    }
}

// generate dual tone audio(left_freq for left channel, right_freq for right channel)
void Audio::generate_dual_tone(short *buffer, short *left_buffer, short *right_buffer, double left_freq, double right_freq, int frames) {
    const double amplitude = 0.5 * 32767.0;   // Half of the maximum amplitude to avoid clipping.
    for (int i = 0; i < frames; i++) {
        double t = (double)i / SAMPLE_RATE;
        double left_sample = amplitude * sin(2.0 * M_PI * left_freq * t);
        double right_sample = amplitude * sin(2.0 * M_PI * right_freq * t);
        left_buffer[i] = (short)left_sample;
        right_buffer[i] = (short)right_sample;
        buffer[i * CHANNELS] = left_buffer[i];
        buffer[i * CHANNELS + 1] = right_buffer[i];
    }
}

// 生成简单的正弦波音频（440Hz，即A4音符）
void Audio::generate_tone(short *buffer, int frames) {
    for (int i = 0; i < frames * CHANNELS; i += CHANNELS) {
        double sample = 0.5 * sin(2.0 * M_PI * 440.0 * i / (SAMPLE_RATE * CHANNELS));
        short value = (short)(sample * 32767);
        
        // 双声道
        buffer[i] = value;
        if (i + 1 < frames * CHANNELS) {
            buffer[i + 1] = value;
        }
    }
}

// 播放音频函数
void Audio::play_audio(const char *device, short *buffer, snd_pcm_uframes_t frames) {
    snd_pcm_t *handle;
    int err;
    
    // 打开PCM设备用于播放
    err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0);
    check_error(err, "Playback open");
    
    // 设置硬件参数
    snd_pcm_hw_params_t *params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(handle, params);
    
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(handle, params, FORMAT);
    snd_pcm_hw_params_set_channels(handle, params, CHANNELS);
    unsigned int sample_rate = SAMPLE_RATE;
    snd_pcm_hw_params_set_rate_near(handle, params, &sample_rate, 0);
    
    err = snd_pcm_hw_params(handle, params);
    check_error(err, "Playback set params");

    log_thread_safe(LOG_LEVEL_INFO, AUDIO_TAG, "Playing audio on device: %s", device);
    
    snd_pcm_uframes_t offset = 0;
    while (offset < frames) {
        snd_pcm_uframes_t frames_to_write = BUFFER_SIZE / CHANNELS;
        if (offset + frames_to_write > frames) {
            frames_to_write = frames - offset;
        }
        
        int err = snd_pcm_writei(handle, buffer + offset * CHANNELS, frames_to_write);
        if (err == -EPIPE) {
            // underrun occurred
            snd_pcm_prepare(handle);
        } else if (err < 0) {
            log_thread_safe(LOG_LEVEL_ERROR, AUDIO_TAG, "Playback write error: %s", snd_strerror(err));
        } else {
            offset += err;
        }
    }
    
    // 等待所有数据播放完成
    snd_pcm_drain(handle);
    log_thread_safe(LOG_LEVEL_INFO, AUDIO_TAG, "Playback finished.");
    
    // 关闭设备
    snd_pcm_close(handle);
}


void Audio::play_audio(int time) {
    // 固定使用ES8388声卡设备
    /*aplay -D plughw:rockchipes8389 /sample.wav */
    // const char *device = "plughw:rockchipes8389"; // 使用ES8388声卡设备名称
    
    // 生成并播放预定义的音频（440Hz正弦波）
    log_thread_safe(LOG_LEVEL_INFO, AUDIO_TAG, "Generating and playing a tone...");
    int tone_frames = SAMPLE_RATE * time; // 3秒的音频
    short *tone_buffer = (short *)malloc(tone_frames * CHANNELS * sizeof(short));
    generate_tone(tone_buffer, tone_frames);
    play_audio(AUDIO_CARD_NAME, tone_buffer, tone_frames);
    free(tone_buffer);
}

// 带音量控制的录制函数
void Audio::recordAudio(int time, short* recorded_buffer, double volume_scale) {
    // 设置音量
    setRecordVolume(volume_scale);
    
    snd_pcm_t *handle;
    int err;
    // const char *device = "plughw:rockchipes8389"; // 使用ES8388声卡设备名称";
    err = snd_pcm_open(&handle, AUDIO_CARD_NAME, SND_PCM_STREAM_CAPTURE, 0);
    check_error(err, "Record open");

    // 设置硬件参数
    snd_pcm_hw_params_t *params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(handle, params);
    
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(handle, params, FORMAT);
    snd_pcm_hw_params_set_channels(handle, params, CHANNELS);
    unsigned int sample_rate = SAMPLE_RATE;
    snd_pcm_hw_params_set_rate_near(handle, params, &sample_rate, 0);
    
    err = snd_pcm_hw_params(handle, params);
    check_error(err, "Record set params");
    
    snd_pcm_uframes_t recorded_frames = 0;

    log_thread_safe(LOG_LEVEL_INFO, AUDIO_TAG, "Recording for %d seconds with volume scale %.2f...", time, volume_scale);

    
    short buffer[BUFFER_SIZE];
    unsigned int total_frames = SAMPLE_RATE * time; // 改为unsigned int避免有符号无符号比较警告
    bool is_recording = true;
    
    // 开始录制
    while (is_recording && recorded_frames < total_frames) {
        int frames = snd_pcm_readi(handle, buffer, BUFFER_SIZE / CHANNELS);
        if (frames < 0) {
            log_thread_safe(LOG_LEVEL_ERROR, AUDIO_TAG, "Record read error: %s", snd_strerror(frames));
            frames = 0;
        } else if (frames > 0) {
            // 将数据复制到全局缓冲区，并应用音量调整
            unsigned int samples_to_copy = frames * CHANNELS;
            if (recorded_frames + frames > total_frames) {
                samples_to_copy = (total_frames - recorded_frames) * CHANNELS;
                frames = total_frames - recorded_frames;
            }
            
            for (unsigned int i = 0; i < samples_to_copy; i++) {
                double sample = buffer[i];
                
                // 应用音量缩放
                double adjusted_sample = sample * record_volume_scale;
                
                // 限幅保护
                if (adjusted_sample > 32767.0) {
                    adjusted_sample = 32767.0;
                } else if (adjusted_sample < -32768.0) {
                    adjusted_sample = -32768.0;
                }
                
                recorded_buffer[recorded_frames * CHANNELS + i] = (short)adjusted_sample;
            }
            recorded_frames += frames;
        }
    }
    log_thread_safe(LOG_LEVEL_INFO, AUDIO_TAG, "Recording finished. Recorded %lu frames.", recorded_frames);
    
    // 关闭设备
    snd_pcm_close(handle);
}

bool Audio::findEventDevice() {
    DIR *dir;
    struct dirent *entry;
    int fd = -1;
    char device_name[256] = "Unknown";
    dir = opendir(DEV_INPUT);
    if (!dir) {
        log_thread_safe(LOG_LEVEL_ERROR, AUDIO_TAG, "无法打开 /dev/input 目录");
        return false;
    }

    while ((entry = readdir(dir)) != NULL) {
        // 检查是否为 event 设备
        if (strncmp(entry->d_name, EVENT_DEV, strlen(EVENT_DEV)) == 0) {
            // 构建完整设备路径
            memset(dev_path, 0, sizeof(dev_path));
            snprintf(dev_path, sizeof(dev_path), "%s/%s", DEV_INPUT, entry->d_name);
            
            // 打开设备文件
            fd = open(dev_path, O_RDONLY);
            if (fd == -1) {
                log_thread_safe(LOG_LEVEL_ERROR, AUDIO_TAG, "无法打开设备文件: %s", dev_path);
                continue;
            }

            // 获取设备名称
            if (ioctl(fd, EVIOCGNAME(sizeof(device_name)), device_name) < 0) {
                log_thread_safe(LOG_LEVEL_ERROR, AUDIO_TAG, "无法获取设备名称: %s", dev_path);
                close(fd);
                continue;
            }

            log_thread_safe(LOG_LEVEL_INFO, AUDIO_TAG, "检查设备: %s -> %s", dev_path, device_name);

            // 检查是否为目标设备
            if (strstr(device_name, AUDIO_EVENT_NAME) != NULL) {
                log_thread_safe(LOG_LEVEL_INFO, AUDIO_TAG, "找到目标设备: %s (%s)", dev_path, device_name);
                closedir(dir);
                close(fd);
                log_thread_safe(LOG_LEVEL_INFO, AUDIO_TAG, "使用设备: %s", dev_path);
                return true;
            }

            // 不是目标设备，关闭并继续查找
            close(fd);
            fd = -1;
        }
    }

    log_thread_safe(LOG_LEVEL_ERROR, AUDIO_TAG, "未找到目标设备: %s", AUDIO_EVENT_NAME);
    closedir(dir);
    return false;
}

void Audio::selectAudioIn() {
    int audioFd = open(dev_path, O_RDONLY);
    if (audioFd < 0) {
        LogError(AUDIO_TAG, "Failed to open audio input device: %s", dev_path);
        return;
    }

    struct input_event ev; 

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(audioFd, &readfds);
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    log_thread_safe(LOG_LEVEL_INFO, AUDIO_TAG, "等待音频设备插入事件， 超时时间: %ld秒", timeout.tv_sec);
    // int ret = select(audioFd + 1, &readfds, NULL, NULL, NULL);
    int ret = select(audioFd + 1, &readfds, NULL, NULL, &timeout);
    if (ret == -1) {
        log_thread_safe(LOG_LEVEL_ERROR, AUDIO_TAG, "select错误: %s", strerror(errno));
        close(audioFd);
        return;
    } 
    else if (ret == 0) {
        log_thread_safe(LOG_LEVEL_WARN, AUDIO_TAG, "select超时,无事件发生");
        close(audioFd);
        return;
    }

    ssize_t n = read(audioFd, &ev, sizeof(ev));
    if (n == -1) {
        log_thread_safe(LOG_LEVEL_ERROR, AUDIO_TAG, "读取事件错误: %s", strerror(errno));
        close(audioFd);
        return;
    }
    // std::cout << "ev.type = " << ev.type << ", ev.code = " << ev.code << ", ev.value = " << ev.value << std::endl;
    log_thread_safe(LOG_LEVEL_INFO, AUDIO_TAG, "读取事件: type=%d, code=%d, value=%d", ev.type, ev.code, ev.value);
    // play_audio(10); // 播放3秒音频
    if (/*ev.type && ev.code && */ev.value == 1) {
        // std::cout << "检测到音频输入设备插入" << std::endl;
        // LogInfo(AUDIO_TAG, "检测到音频输入设备插入");
        log_thread_safe(LOG_LEVEL_INFO, AUDIO_TAG, "检测到音频输入设备插入");
        play_audio(3); // 播放3秒音频
    }

    close(audioFd);
}



void Audio::save_buffer_to_wav(const char *filename, short *buffer, int frames) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        log_thread_safe(LOG_LEVEL_ERROR, AUDIO_TAG, "cannot open file to write: %s", filename);
        return;
    }

    // wav file header structure
    struct WavHeader {
        char chunkId[4] = {'R', 'I', 'F', 'F'};
        uint32_t chunkSize;
        char format[4] = {'W', 'A', 'V', 'E'};
        char subchunk1Id[4] = {'f', 'm', 't', ' '};
        uint32_t subchunk1Size = 16;
        uint16_t audioFormat = 1; // PCM
        uint16_t numChannels = CHANNELS;
        uint32_t sampleRate = SAMPLE_RATE;
        uint32_t byteRate = SAMPLE_RATE * CHANNELS * sizeof(short);
        uint16_t blockAlign = CHANNELS * sizeof(short);
        uint16_t bitsPerSample = 16;
        char subchunk2Id[4] = {'d', 'a', 't', 'a'};
        uint32_t subchunk2Size;
    } header;

    // conculate data size
    header.subchunk2Size = frames * CHANNELS * sizeof(short);
    header.chunkSize = 36 + header.subchunk2Size;

    // write header to file
    fwrite(&header, sizeof(header), 1, file);

    // write audio data to file
    fwrite(buffer, sizeof(short), frames * CHANNELS, file);

    fclose(file);

    log_thread_safe(LOG_LEVEL_INFO, AUDIO_TAG, "audio data saved to file: %s (frames: %d)", filename, frames);
}

void Audio::recordAudio(int time) {
    int tone_frames = SAMPLE_RATE * time;
    short *recorded_buffer = (short *)malloc(tone_frames * CHANNELS * sizeof(short));
    recordAudio(time, recorded_buffer);
    
    // 保存录制数据到文件

    
    play_audio("plughw:rockchipes8389", recorded_buffer, tone_frames);
    free(recorded_buffer);
}

void Audio::recordAudio(int time, short* recorded_buffer) {
    // 直接调用带音量控制的版本，使用当前的音量缩放比例
    recordAudio(time, recorded_buffer, record_volume_scale);
}

// calculate RMS of the signal, used to measure signal strength
double Audio::calculate_signal_rms(short *signal, int samples) {
    double sum_squares = 0.0;
    int valid_samples = 0;
    
    for (int i = 0; i < samples; i++) {
        double sample = signal[i] / 32768.0; // uniformization to [-1, 1]
        sum_squares += sample * sample;
        valid_samples++;
    }
    
    if (valid_samples == 0) return 0.0;
    
    return sqrt(sum_squares / valid_samples);
}

// calculate peak of the signal, for more precise gain calculation
double Audio::calculate_signal_peak(short *signal, int samples) {
    double max_peak = 0.0;
    
    for (int i = 0; i < samples; i++) {
        double abs_sample = fabs(signal[i] / 32768.0); // uniformization to [-1, 1]
        if (abs_sample > max_peak) {
            max_peak = abs_sample;
        }
    }
    
    return max_peak;
}

// 计算自动增益值 - 改进版本
double Audio::calculate_auto_gain(double play_rms, double record_rms,
                               double play_peak, double record_peak) {
    if (record_rms < 1e-6) return 8.0; // 默认增益，避免除零
    
    // 使用RMS和峰值结合计算增益
    double rms_gain = play_rms / record_rms;
    double peak_gain = (record_peak > 1e-6) ? (play_peak / record_peak) : rms_gain;
    
    // 结合RMS增益和峰值增益，偏向于更保守的增益
    double target_gain = (rms_gain + peak_gain) / 2.0;
    
    // 如果录制信号非常小，应用额外的放大
    if (record_rms < 0.01) {
        target_gain *= 2.0; // 对小信号应用额外放大
    }
    
    // 限制增益范围在合理范围内 (1.0 到 50.0)
    if (target_gain < 1.0) target_gain = 1.0;
    if (target_gain > 50.0) target_gain = 50.0;
    
    // printf("自动增益计算: 播放RMS=%.6f, 录制RMS=%.6f, 增益=%.2f\n",
    //        play_rms, record_rms, target_gain);
    // printf("峰值信息: 播放峰值=%.6f, 录制峰值=%.6f\n", play_peak, record_peak);
    log_thread_safe(LOG_LEVEL_INFO, AUDIO_TAG, "自动增益计算: 播放RMS=%.6f, 录制RMS=%.6f, 播放峰值=%.6f, 录制峰值=%.6f, 增益=%.2f",
                         play_rms, record_rms, play_peak, record_peak, target_gain);
    
    return target_gain;
}

// 应用增益到音频缓冲区
void Audio::apply_gain_for_buffer(short *buffer, int samples, double gain) {
    for (int i = 0; i < samples; i++) {
        double sample = buffer[i] * gain;
        
        // 限幅保护
        if (sample > 32767.0) {
            sample = 32767.0;
        } else if (sample < -32768.0) {
            sample = -32768.0;
        }
        
        buffer[i] = (short)sample;
    }
}


void Audio::recordAndCompare(int time) {
    printf("=== 音频录制与播放测试开始 ===\n");
    printf("测试时长: %d秒\n", time);
    printf("功能: 播放音频、录制音频、自动增益、生成音频文件\n\n");

    int tone_frames = SAMPLE_RATE * time;
    short *tone_buffer = (short *)malloc(tone_frames * CHANNELS * sizeof(short));
    short *recorded_buffer = (short *)malloc(tone_frames * CHANNELS * sizeof(short));
    short *adjusted_buffer = (short *)malloc(tone_frames * CHANNELS * sizeof(short));
    
    // 分离左右声道缓冲区
    short *left_channel_buffer = (short *)malloc(tone_frames * sizeof(short));
    short *right_channel_buffer = (short *)malloc(tone_frames * sizeof(short));
    
    // 生成测试音频
    printf("生成测试音频...\n");
    generate_tone(tone_buffer, tone_frames);
    
    // 分离左右声道
    printf("分离左右声道...\n");
    for (int i = 0; i < tone_frames; i++) {
        left_channel_buffer[i] = tone_buffer[i * CHANNELS];        // 左声道
        right_channel_buffer[i] = tone_buffer[i * CHANNELS + 1];   // 右声道
    }
    
    // 扫描播放音频参数
    printf("扫描播放音频参数...\n");
    double play_rms = calculate_signal_rms(tone_buffer, tone_frames * CHANNELS);
    double play_peak = calculate_signal_peak(tone_buffer, tone_frames * CHANNELS);
    printf("播放音频 - RMS: %.6f, 峰值: %.6f\n", play_rms, play_peak);
    
    // 进行一次录制
    printf("开始录制音频...\n");
    
    // 使用同步机制确保播放和录制同时开始
    std::mutex mtx;
    std::condition_variable cv;
    bool ready_to_start = false;

    std::thread play_thread([this, tone_buffer, tone_frames, &mtx, &cv, &ready_to_start]() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&ready_to_start] { return ready_to_start; });
        lock.unlock(); // 释放锁后再执行播放
        play_audio("plughw:rockchipes8389", tone_buffer, tone_frames);
    });

    // 创建线程但不立即获取锁
    std::thread record_thread([this, time, recorded_buffer, &mtx, &cv, &ready_to_start]() {
        // 在lambda内部获取锁并等待
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&ready_to_start] { return ready_to_start; });
        lock.unlock(); // 释放锁后再执行录制
        recordAudio(time, recorded_buffer);
    });

    // 等待线程启动并进入等待状态
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // 同时启动播放和录制线程
    {
        std::lock_guard<std::mutex> lock(mtx);
        ready_to_start = true;
    }
    cv.notify_all();

    record_thread.join();
    play_thread.join();
    
    printf("录制完成，开始分析录制音频...\n");
    // play_audio("plughw:rockchipes8389", recorded_buffer, tone_frames);
    
    // 扫描录制音频参数
    double record_rms = calculate_signal_rms(recorded_buffer, tone_frames * CHANNELS);
    double record_peak = calculate_signal_peak(recorded_buffer, tone_frames * CHANNELS);
    
    printf("录制音频: RMS=%.6f, 峰值=%.6f\n", record_rms, record_peak);
    
    // 自动计算增益
    double auto_gain = calculate_auto_gain(play_rms, record_rms, play_peak, record_peak);
    
    printf("自动增益计算: 增益=%.2f\n", auto_gain);
    
    // 复制录制音频到调整缓冲区
    memcpy(adjusted_buffer, recorded_buffer, tone_frames * CHANNELS * sizeof(short));
    
    // 应用增益到调整缓冲区
    printf("应用增益到录制音频...\n");
    apply_gain_for_buffer(adjusted_buffer, tone_frames * CHANNELS, auto_gain);
    
    // 计算调整后音频的RMS和峰值
    double adjusted_rms = calculate_signal_rms(adjusted_buffer, tone_frames * CHANNELS);
    double adjusted_peak = calculate_signal_peak(adjusted_buffer, tone_frames * CHANNELS);
    
    printf("\n增益调整结果:\n");
    printf("原始录制: RMS=%.6f, 峰值=%.6f\n", record_rms, record_peak);
    printf("调整录制: RMS=%.6f, 峰值=%.6f\n", adjusted_rms, adjusted_peak);
    printf("参考播放: RMS=%.6f, 峰值=%.6f\n", play_rms, play_peak);
    
    // 提取调整后音频的左声道
    short *adjusted_left_channel = (short *)malloc(tone_frames * sizeof(short));
    for (int i = 0; i < tone_frames; i++) {
        adjusted_left_channel[i] = adjusted_buffer[i * CHANNELS];
    }
    
    // 实现三个比较算法
    printf("\n=== 音频比较分析 ===\n");
    
    // 1. 样本差 RMS
    auto rms_difference = [](const short *a, const short *b, int n) {
        double sum = 0.0;
        for (int i = 0; i < n; i++) {
            double d = (double)a[i] - (double)b[i];
            sum += d * d;
        }
        return sqrt(sum / n);
    };
    
    // 2. 交叉相关（归一化相关系数）
    auto cross_correlation = [](const short *x, const short *y, int n) {
        double sum_xy = 0, sum_x = 0, sum_y = 0;
        double sum_x2 = 0, sum_y2 = 0;

        for (int i = 0; i < n; i++) {
            sum_xy += x[i] * y[i];
            sum_x  += x[i];
            sum_y  += y[i];
            sum_x2 += x[i] * x[i];
            sum_y2 += y[i] * y[i];
        }

        double numerator = n * sum_xy - sum_x * sum_y;
        double denominator =
            sqrt((n * sum_x2 - sum_x * sum_x) *
                 (n * sum_y2 - sum_y * sum_y));

        if (denominator == 0) return 0.0;
        return numerator / denominator;     // -1 ~ 1
    };
    
    // 3. 频谱余弦相似度（简化版，使用幅度谱）
    auto spectral_cosine_similarity = [](const short *a, const short *b, int n) {
        // 计算幅度谱（简化版本，使用绝对值代替FFT）
        double dot = 0, na = 0, nb = 0;

        for (int i = 0; i < n; i++) {
            double pa = fabs((double)a[i]);
            double pb = fabs((double)b[i]);

            dot += pa * pb;
            na  += pa * pa;
            nb  += pb * pb;
        }

        double cosine = 0;
        if (na > 0 && nb > 0)
            cosine = dot / (sqrt(na) * sqrt(nb));

        return cosine;
    };
    
    // 与左声道比较
    printf("\n--- 与左声道比较 ---\n");
    double rms_diff_left = rms_difference(adjusted_left_channel, left_channel_buffer, tone_frames);
    double cross_corr_left = cross_correlation(adjusted_left_channel, left_channel_buffer, tone_frames);
    double spectral_cosine_left = spectral_cosine_similarity(adjusted_left_channel, left_channel_buffer, tone_frames);
    
    printf("样本差 RMS: %.6f\n", rms_diff_left);
    printf("交叉相关: %.6f\n", cross_corr_left);
    printf("频谱余弦相似度: %.6f\n", spectral_cosine_left);

    // 与右声道比较
    printf("\n--- 与右声道比较 ---\n");
    double rms_diff_right = rms_difference(adjusted_left_channel, right_channel_buffer, tone_frames);
    double cross_corr_right = cross_correlation(adjusted_left_channel, right_channel_buffer, tone_frames);
    double spectral_cosine_right = spectral_cosine_similarity(adjusted_left_channel, right_channel_buffer, tone_frames);

    printf("样本差 RMS: %.6f\n", rms_diff_right);
    printf("交叉相关: %.6f\n", cross_corr_right);
    printf("频谱余弦相似度: %.6f\n", spectral_cosine_right);

    
    // 与总声道比较
    printf("\n--- 与总声道比较 ---\n");
    double rms_diff_total = rms_difference(adjusted_buffer, tone_buffer, tone_frames * CHANNELS);
    double cross_corr_total = cross_correlation(adjusted_buffer, tone_buffer, tone_frames * CHANNELS);
    double spectral_cosine_total = spectral_cosine_similarity(adjusted_buffer, tone_buffer, tone_frames * CHANNELS);
    
    printf("样本差 RMS: %.6f\n", rms_diff_total);
    printf("交叉相关: %.6f\n", cross_corr_total);
    printf("频谱余弦相似度: %.6f\n", spectral_cosine_total);
    
    // 保存音频文件
    printf("\n=== 生成音频文件 ===\n");
    save_buffer_to_wav("recorded_audio.wav", recorded_buffer, tone_frames);
    save_buffer_to_wav("reference_tone.wav", tone_buffer, tone_frames);
    save_buffer_to_wav("adjusted_audio.wav", adjusted_buffer, tone_frames);
    printf("音频文件已保存: recorded_audio.wav, reference_tone.wav, adjusted_audio.wav\n");
    
    // 释放内存
    free(tone_buffer);
    free(recorded_buffer);
    free(adjusted_buffer);
    free(left_channel_buffer);
    free(right_channel_buffer);
    free(adjusted_left_channel);
    
    printf("\n=== 音频录制与播放测试完成 ===\n");
}

// 设置录制音量缩放比例
void Audio::setRecordVolume(double volume_scale) {
    // 调整音量范围到0.0到10.0之间，更合理的范围
    if (volume_scale < 0.0) {
        record_volume_scale = 0.0;
    } else if (volume_scale > 10.0) {
        record_volume_scale = 10.0;
    } else {
        record_volume_scale = volume_scale;
    }
    // printf("设置录制音量缩放比例: %.2f\n", record_volume_scale);
    LogInfo(AUDIO_TAG, "设置录制音量缩放比例: %.2f", record_volume_scale);
}

// 获取当前录制音量缩放比例
double Audio::getRecordVolume() const {
    return record_volume_scale;
}



int Audio::record_and_compare_audio_v2(int time) {
    // char * device = "plughw:rockchipes8389"; // use ES8388 sound card device name

    int tone_frames = SAMPLE_RATE * time;
    short *play_tone_buffer = (short *)malloc(tone_frames * CHANNELS * sizeof(short));
    short *play_left_buffer = (short *)malloc(tone_frames * sizeof(short));
    short *play_right_buffer = (short *)malloc(tone_frames * sizeof(short));

    short *recorded_buffer = (short *)malloc(tone_frames * CHANNELS * sizeof(short));

    short *adjusted_left_buffer = (short *)malloc(tone_frames * sizeof(short));
    short *adjusted_right_buffer = (short *)malloc(tone_frames * sizeof(short));

    generate_dual_tone(play_tone_buffer, play_left_buffer, play_right_buffer, left_freq, right_freq, tone_frames);
    log_thread_safe(LOG_LEVEL_INFO, AUDIO_TAG, "generating dual-tone test audio, left: %.2f Hz, right: %.2f Hz", left_freq, right_freq);

    save_buffer_to_wav("dual_tone.wav", play_tone_buffer, tone_frames);
    save_buffer_to_wav("play_left_channel.wav", play_left_buffer, tone_frames);
    save_buffer_to_wav("play_right_channel.wav", play_right_buffer, tone_frames);
    log_thread_safe(LOG_LEVEL_INFO, AUDIO_TAG, "dual-tone test audio saved to dual_tone.wav");
    
    // use synchronization mechanism to ensure playback and recording start simultaneously
    std::mutex mtx;
    std::condition_variable cv;
    bool ready_to_start = false;
    std::thread play_thread([this, play_tone_buffer, tone_frames, &mtx, &cv, &ready_to_start]() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&ready_to_start] { return ready_to_start; });
        lock.unlock(); 
        play_audio(AUDIO_CARD_NAME, play_tone_buffer, tone_frames);
    });

    std::thread record_thread([this, time, recorded_buffer, &mtx, &cv, &ready_to_start]() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&ready_to_start] { return ready_to_start; });
        lock.unlock(); 
        recordAudio(time, recorded_buffer);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::lock_guard<std::mutex> lock(mtx);
    ready_to_start = true;
    cv.notify_all();
    record_thread.join();
    play_thread.join();
    
    save_buffer_to_wav("recorded_dual_tone.wav", recorded_buffer, tone_frames);

    double play_rms = calculate_signal_rms(play_tone_buffer, tone_frames * CHANNELS);
    double record_rms = calculate_signal_rms(recorded_buffer, tone_frames * CHANNELS);
    double play_peak = calculate_signal_peak(play_tone_buffer, tone_frames * CHANNELS);
    double record_peak = calculate_signal_peak(recorded_buffer, tone_frames * CHANNELS);
    double auto_gain = calculate_auto_gain(play_rms, record_rms, play_peak, record_peak);
    apply_gain_for_buffer(recorded_buffer, tone_frames * CHANNELS, auto_gain);
    save_buffer_to_wav("adjusted_dual_tone.wav", recorded_buffer, tone_frames);

    free(play_tone_buffer);
    free(play_left_buffer);
    free(play_right_buffer);

    free(recorded_buffer);
    free(adjusted_left_buffer);
    free(adjusted_right_buffer);

    std::vector<double> samples;
    uint32_t sr;
    int top_k = 2;
    if (!read_wav_simple("adjusted_dual_tone.wav", samples, sr)) {
        log_thread_safe(LOG_LEVEL_ERROR, AUDIO_TAG, "Failed to read WAV file: adjusted_dual_tone.wav");
        // return -1;
    }

    log_thread_safe(LOG_LEVEL_INFO, AUDIO_TAG, "Loaded %zu samples, SR=%u", samples.size(), sr);
    return detect_main_frequencies(samples, sr, top_k);
}


bool Audio::read_wav_simple(const string &path,
                     vector<double> &samples,
                     uint32_t &sample_rate)
{
    ifstream f(path, ios::binary);
    if (!f) return false;

    char riff[4], wave[4];
    uint32_t size;

    // RIFF
    f.read(riff, 4);
    f.read(reinterpret_cast<char*>(&size), 4);
    f.read(wave, 4);

    if (strncmp(riff, "RIFF", 4) != 0 || strncmp(wave, "WAVE", 4) != 0) {
        cerr << "Not a valid WAV file\n";
        return false;
    }

    uint16_t channels = 1;
    uint16_t bits = 16;
    uint32_t data_pos = 0;
    uint32_t data_size = 0;

    // 扫描所有 chunk，找到 fmt 和 data
    while (!f.eof()) {
        char id[4];
        uint32_t chunk_size;

        f.read(id, 4);
        if (!f) break;
        f.read(reinterpret_cast<char*>(&chunk_size), 4);
        if (!f) break;

        if (strncmp(id, "fmt ", 4) == 0) {
            vector<uint8_t> buf(chunk_size);
            f.read(reinterpret_cast<char*>(buf.data()), chunk_size);

            channels    = buf[2] | (buf[3] << 8);
            sample_rate = buf[4] | (buf[5] << 8) | (buf[6] << 16) | (buf[7] << 24);
            bits        = buf[14] | (buf[15] << 8);
        }
        else if (strncmp(id, "data", 4) == 0) {
            data_size = chunk_size;
            data_pos = f.tellg();
            f.seekg(chunk_size, ios::cur);
        }
        else {
            // skip 其他 chunk
            f.seekg(chunk_size, ios::cur);
        }
    }

    if (data_size == 0) {
        cerr << "No data chunk\n";
        return false;
    }

    // 读取 PCM
    f.clear();
    f.seekg(data_pos, ios::beg);

    uint32_t total_samples = (data_size * 8) / bits;
    uint32_t frames = total_samples / channels;

    samples.reserve(frames);

    for (uint32_t i = 0; i < frames; i++) {
        double mix = 0;

        for (int ch = 0; ch < channels; ch++) {
            if (bits == 16) {
                int16_t s;
                f.read(reinterpret_cast<char*>(&s), 2);
                mix += s / 32768.0;
            } else if (bits == 24) {
            unsigned char b[3];
            f.read(reinterpret_cast<char*>(b), 3);

            // 24bit little-endian 转为 int32
            int32_t s = (b[0] | (b[1] << 8) | (b[2] << 16));
            // 手动符号扩展
            if (s & 0x800000) s |= ~0xFFFFFF;

            mix += s / 8388608.0;  // 2^23
            }  else if (bits == 32) {
                int32_t s;
                f.read(reinterpret_cast<char*>(&s), 4);
                mix += s / 2147483648.0;
            } else {
                cerr << "Unsupported bits: " << bits << endl;
                return false;
            }
        }

        samples.push_back(mix / channels);
    }

    return true;
}

/* ----------- Hann 窗 ----------- */
void Audio::apply_hann(vector<double> &x)
{
    int N = x.size();
    for (int n = 0; n < N; n++) {
        x[n] *= 0.5 * (1 - cos(2 * M_PI * n / (N - 1)));
    }
}

/* ----------- 找主要频率 ----------- */
int Audio::detect_main_frequencies(const vector<double> &samples,
                             uint32_t sr, int top_k)
{
    int N = samples.size();

    int Nfft = 1;
    while (Nfft < N) Nfft <<= 1;
    if (Nfft > 262144) Nfft = 262144;  // 限制最大 FFT 长度

    vector<double> buf = samples;
    buf.resize(Nfft, 0);
    apply_hann(buf);

    fftw_complex *out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (Nfft/2 + 1));
    fftw_plan plan = fftw_plan_dft_r2c_1d(Nfft, buf.data(), out, FFTW_ESTIMATE);
    fftw_execute(plan);

    vector<double> mag(Nfft/2+1);
    for (int i = 0; i <= Nfft/2; i++) {
        double re = out[i][0], im = out[i][1];
        mag[i] = sqrt(re*re + im*im);
    }

    fftw_destroy_plan(plan);
    fftw_free(out);

    // 归一化
    double mmax = *max_element(mag.begin(), mag.end());
    for (double &v : mag) v /= mmax;

    // 找峰值
    vector<pair<double,double>> peaks;
    for (int i = 1; i < Nfft/2; i++) {
        if (mag[i] > mag[i-1] && mag[i] > mag[i+1]) {
            double freq = i * (double)sr / Nfft;
            peaks.emplace_back(freq, mag[i]);
        }
    }

    sort(peaks.begin(), peaks.end(),
         [](auto &a, auto &b) { return a.second > b.second; });

    cout << "Top " << top_k << " frequencies:\n";
    for (size_t  i = 0; i < static_cast<size_t>(top_k) && i < peaks.size(); i++) {
        cout << fixed << setprecision(2)
             << peaks[i].first << " Hz (mag=" << peaks[i].second << ")\n";
    }

    // 判断是否找到了预期频率
    // double left_freq_min = 230;
    // double left_freq     = 250;  
    // double left_freq_max = 270; 

    // double right_freq_min = 380; 
    // double right_freq     = 400; 
    // double right_freq_max = 420; 
    int left_found = 0x00;
    int right_found = 0x00;

    for (size_t i = 0; i < static_cast<size_t>(top_k) && i < peaks.size(); i++) {
        double freq = peaks[i].first;
        if (freq >= left_freq - 20 && freq <= left_freq + 20) {
            log_thread_safe(LOG_LEVEL_INFO, AUDIO_TAG, "detect left channel frequency: %.2f Hz", freq);
            left_found = 0x02;  // 0000 0010
        }
        if (freq >= right_freq - 20 && freq <= right_freq + 20) {
            log_thread_safe(LOG_LEVEL_INFO, AUDIO_TAG, "detect right channel frequency: %.2f Hz", freq);
            right_found = 0x01;  // 0000 0001
        }
    }

    return left_found | right_found;
}
