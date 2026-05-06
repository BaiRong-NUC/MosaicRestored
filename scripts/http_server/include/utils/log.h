#include "utils/public.h"
#pragma once

// Log.h定义了一个简单的日志系统,使用宏LOG来输出日志信息,并包含日志级别和文件位置信息,时间信息.
enum LogLevel
{
    INFO,
    WARNING,
    ERROR
};

// #define LOG_LEVEL INFO // 设置默认日志级别为INFO,即输出INFO、WARNING和ERROR级别的日志

extern LogLevel LOG_LEVEL;
void SetLogLevel(LogLevel level);

// 将日志级别转换为字符串
std::string LogLevelToString(LogLevel level);

// 获取相对文件路径,如果文件路径中包含"HttpServer/",则返回相对于"HttpServer/"的路径,否则返回原始路径
std::string GetRelativeFile(const char *file);

// 获取日志打印时间
std::string GetCurrentTime();

// void LOG(LogLevel level, const std::string &msg);
#define LOG(level, msg) if ((level) >= LOG_LEVEL) std::cout << "[" << GetCurrentTime() << " " << LogLevelToString(level) << " " << GetRelativeFile(__FILE__) << ":" << __LINE__ << "] " << msg << std::endl