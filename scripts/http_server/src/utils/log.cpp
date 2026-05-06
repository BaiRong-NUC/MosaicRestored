#include "utils/log.h"

LogLevel LOG_LEVEL = WARNING;
void SetLogLevel(LogLevel level) { LOG_LEVEL = level; }

std::string LogLevelToString(LogLevel level)
{
    switch (level)
    {
    case INFO:
        return "INFO";
    case WARNING:
        return "WARNING";
    case ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

std::string GetRelativeFile(const char *file)
{
    const char *pos = strstr(file, "HttpServer/");
    if (pos)
    {
        return std::string("~/") + (pos + strlen("HttpServer/"));
    }
    return file;
}

std::string GetCurrentTime()
{
    time_t now = time(nullptr);
    struct tm *tm_info = localtime(&now);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%m-%d %H:%M:%S", tm_info);
    return std::string(buffer);
}

// void LOG(LogLevel level, const std::string &msg)
// {
//     if (level < LOG_LEVEL)
//         return;
//     std::cout << "["
//               << GetCurrentTime() << " "
//               << LogLevelToString(level) << " "
//               << GetRelativeFile(__FILE__) << ":" << __LINE__
//               << "] "
//               << msg << std::endl;
// }