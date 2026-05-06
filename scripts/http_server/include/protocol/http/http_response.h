#pragma once
#include <utils/public.h>
#include <utils/utils.h>
/**
 * HTTP响应构建与管理，支持设置状态码、响应头部和消息体。
 * 1. 存储HTTP响应信息
 * 2. HTTP格式:
 *      状态行: VERSION STATUS_CODE REASON_PHRASE\r\n
 *      响应头部: Header-Name: Header-Value\r\n
 *      空行: \r\n
 *      正文: 可选,根据Content-Length或Transfer-Encoding确定长度
 * 3. 方法:
 *      - 头部字段的设置与查询
 *      - 正文的设置,获取
 *      - 重定向的设置
 */
class HttpResponse
{
   public:
    int status_code;                                       // 状态码
    std::unordered_map<std::string, std::string> headers;  // 响应头部字段
    std::string body;                                      // 响应消息体
    bool is_redirect;                                      // 是否是重定向响应
    std::string redirect_location;                         // 重定向目标URL
    std::string version;                                   // HTTP版本,默认为HTTP/1.1

    HttpResponse(int status = 200);  // 默认状态码为200 OK,非重定向响应

    // 设置头部字段
    void SetHeader(const std::string &key, const std::string &value);
    // 判断是否有某个头部字段
    bool HasHeader(const std::string &key) const;
    // 获取头部字段的值,如果不存在则返回空字符串
    std::string GetHeader(const std::string &key) const;
    // 设置响应消息体,并且设置Content-Type头部字段,默认为"text/html"
    void SetBody(const std::string &body, const std::string &content_type = "text/html");
    // 获取响应消息体
    std::string GetBody() const;
    // 设置重定向响应,location为重定向目标URL,status_code默认为302 Found
    void SetRedirect(const std::string &location, int status_code = 302);

    // 重置
    void Clear();

    // 构造HTTP响应报文字符串
    std::string ToString(bool include_body = true) const;
};