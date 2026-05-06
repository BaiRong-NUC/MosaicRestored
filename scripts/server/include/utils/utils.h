#pragma once
#include "utils/public.h"
#include "utils/buffer.h"
#include "utils/log.h"

class Utils
{
   public:
    static std::unordered_map<int, std::string> status_messages;
    static std::unordered_map<std::string, std::string> mime_types;
    // 获取文件内容
    static bool GetFileContent(const std::string &file_name, Buffer *buffer);
    // 写文件
    static bool WriteFileContent(const char *file, const std::string &content);
    // URL编码,避免url中的资源路径名与查询字符串的特殊字符冲突,产生歧义
    // RFC3986 规定 .*_~字母,数字不编码
    // W3C规定 空格需要编码为+
    static std::string UrlEncode(const std::string &str, bool encode_space_as_plus = true);
    // URL解码
    static std::string UrlDecode(const std::string &str, bool decode_plus_as_space = true);
    // 相应状态码信息获取
    static std::string GetStatusMessage(int status_code);
    // 根据文件后缀名获取文件的mime类型
    static std::string GetMimeType(const std::string &file_name);
    // 判断文件是否是一个目录
    static bool IsDirectory(const std::string &path);
    // 判断是否是文件
    static bool IsFile(const std::string &path);
    // 判断请求路径是否合法
    static bool IsValidPath(const std::string &path);
    // 字符串分隔
    static std::vector<std::string> Split(const std::string &str, const std::string &delimiter,
                                          bool ignore_empty = false);
};