#ifndef __LOG_H__
#define __LOG_H__

#include <chrono>
#include <iostream>
#include <string>

enum LogLevel {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_ERROR = 3
};

const LogLevel defaultLogLevel = LOG_LEVEL_DEBUG;

std::string get_current_time();

void LogDebug(const char* TAG, const char* format, ...);
void LogInfo(const char* TAG, const char* format, ...);
void LogWarn(const char* TAG, const char* format, ...);
void LogError(const char* TAG, const char* format, ...);

void log_output_str(LogLevel level, const char* TAG, const std::string& msg);
void log_thread_safe(LogLevel level, const char* TAG, const char* format, ...);

#endif 