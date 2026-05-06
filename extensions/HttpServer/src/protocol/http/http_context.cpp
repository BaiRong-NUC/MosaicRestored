#include <protocol/http/http_context.h>
#include <algorithm>
#include <cctype>

HttpContext::HttpContext() : _response_status(200), _accept_status(HttpAcceptStatus::ACCEPTING_REQUEST_LINE)
{
    this->_http_request_line_re = std::regex("^([A-Z]+)\\s+(\\S+)\\s+HTTP/([0-9\\.]+)$", std::regex::icase);
}

int HttpContext::GetResponseStatus() const { return this->_response_status; }
HttpAcceptStatus HttpContext::GetAcceptStatus() const { return this->_accept_status; }
HttpRequest &HttpContext::GetRequest() { return this->_request; }

// 1. 解析请求行
bool HttpContext::_ParseRequestLine(Buffer *buffer)
{
    std::string request_line = buffer->ReadLine(false);  // 不足一行先不读出来
    auto readSize = buffer->GetReadableSize();
    if (request_line.empty())
    {
        if (readSize > MAX_LINE_SIZE)
        {
            // 请求行过长,超过最大限制,返回414 Request-URI Too Lon{g错误
            this->_response_status = 414;
            this->_accept_status = HttpAcceptStatus::ACCEPTING_ERROR;
            return false;
        }
        // 数据不为空,但没有完整的请求行,继续等待数据
        this->_accept_status = HttpAcceptStatus::ACCEPTING_REQUEST_LINE;
        return true;
    }
    // 去掉行尾可能存在的 '\r'（处理 CRLF）
    if (!request_line.empty() && request_line.back() == '\r')
    {
        request_line.pop_back();
    }
    // 读取到完整的请求行
    if (request_line.size() > MAX_LINE_SIZE)
    {
        // 请求行过长,超过最大限制,返回414 Request-URI Too Long错误
        this->_response_status = 414;
        this->_accept_status = HttpAcceptStatus::ACCEPTING_ERROR;
        return false;
    }
    // 解析请求行,提取请求方法、URI和HTTP版本等信息

    std::smatch matchs;
    if (std::regex_match(request_line, matchs, this->_http_request_line_re))
    {
        std::string method = matchs[1].str();
        std::transform(method.begin(), method.end(), method.begin(), ::toupper);
        this->_request.method = method;          // 请求方法,存为全大写
        std::string full_uri = matchs[2].str();  // 包含 path 和可选 query
        // 分离 path 与 query,这里可能处理多个?,把第一个之后的都当作 query
        size_t qpos = full_uri.find('?');
        std::string path = (qpos == std::string::npos) ? full_uri : full_uri.substr(0, qpos);
        std::string query_str = (qpos == std::string::npos) ? "" : full_uri.substr(qpos + 1);
        this->_request.uri = Utils::UrlDecode(path, false);

        if (!query_str.empty())
        {
            // 先按 '&' 拆分每个参数片段，再对 key/value 单独解码
            std::vector<std::string> parts = Utils::Split(query_str, "&");
            for (const auto &param : parts)
            {
                if (param.empty()) continue;
                size_t eq_pos = param.find('=');
                if (eq_pos != std::string::npos)
                {
                    std::string key = param.substr(0, eq_pos);
                    std::string value = param.substr(eq_pos + 1);
                    key = Utils::UrlDecode(key, true);
                    value = Utils::UrlDecode(value, true);
                    this->_request.query_params[key] = value;
                }
                else
                {
                    // 没有 '=', 当作 key 但值为空
                    std::string key = Utils::UrlDecode(param, true);
                    this->_request.query_params[key] = "";
                }
            }
        }
        this->_request.version = std::string("HTTP/") + matchs[3].str();  // HTTP版本,存为HTTP/x.y
    }
    else
    {
        // 请求行格式错误,返回400 Bad Request错误
        this->_response_status = 400;
        this->_accept_status = HttpAcceptStatus::ACCEPTING_ERROR;
        return false;
    }

    // 请求行解析成功,进入解析请求头部阶段
    this->_accept_status = HttpAcceptStatus::ACCEPTING_HEADERS;
    return true;
}

// 2. 解析请求头部直到遇到空行
// 注意: Buffer::ReadLine(false) 在遇到 CRLF 时会返回一个只含"\r"的字符串，
// 所以要先去掉尾部的 '\r' 再判断是否为空行 ReadLine 返回空字符串表示还没读到换行符。
bool HttpContext::_ParseHeaders(Buffer *buffer)
{
    if (this->_accept_status != HttpAcceptStatus::ACCEPTING_HEADERS)
    {
        return false;
    }

    auto trim = [](std::string &s)
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    };

    while (true)
    {
        std::string header_line = buffer->ReadLine(false);
        auto readSize2 = buffer->GetReadableSize();

        // 没有完整行(没有遇到 '\n')——继续等待数据
        if (header_line.empty())
        {
            if (readSize2 > MAX_LINE_SIZE)
            {
                // 头部行过长,超过最大限制,返回414错误
                this->_response_status = 414;
                this->_accept_status = HttpAcceptStatus::ACCEPTING_ERROR;
                return false;
            }
            this->_accept_status = HttpAcceptStatus::ACCEPTING_HEADERS;
            return true;
        }

        // 去掉行尾可能存在的 '\r'(处理 CRLF)
        if (!header_line.empty() && header_line.back() == '\r')
        {
            header_line.pop_back();
        }

        // 去掉 CR 后如果为空，说明这是一个空行（\r\n），表示头部结束
        if (header_line.empty())
        {
            break;  // 头部解析完成
        }

        // 检查头部行长度是否超限
        if (header_line.size() > MAX_LINE_SIZE)
        {
            this->_response_status = 414;  // Header line too long
            this->_accept_status = HttpAcceptStatus::ACCEPTING_ERROR;
            return false;
        }

        // 解析 Header: Key: Value
        size_t colon_pos = header_line.find(':');
        if (colon_pos == std::string::npos)
        {
            // 头部格式错误
            this->_response_status = 400;  // Bad Request错误
            this->_accept_status = HttpAcceptStatus::ACCEPTING_ERROR;
            return false;
        }
        std::string key = header_line.substr(0, colon_pos);
        std::string value = header_line.substr(colon_pos + 1);
        trim(key);
        trim(value);
        this->_request.SetHeader(key, value);
    }
    // 头部解析完成,进入解析请求正文阶段
    this->_accept_status = HttpAcceptStatus::ACCEPTING_BODY;
    return true;
}

// 3. 头部解析完成,处理可能的正文
bool HttpContext::_ParseBody(Buffer *buffer)
{
    if (this->_accept_status != HttpAcceptStatus::ACCEPTING_BODY)
    {
        return false;
    }

    // 如果存在 Transfer-Encoding: chunked, 暂不支持
    if (this->_request.HasHeader("Transfer-Encoding"))
    {
        std::string te = this->_request.GetHeader("Transfer-Encoding");
        std::string te_low = te;
        std::transform(te_low.begin(), te_low.end(), te_low.begin(), [](unsigned char c) { return std::tolower(c); });
        if (te_low.find("chunked") != std::string::npos)
        {
            this->_response_status = 501;  // 未实现分块传输解析
            this->_accept_status = HttpAcceptStatus::ACCEPTING_ERROR;
            return false;
        }
    }

    size_t content_length = this->_request.GetBodyLength();
    if (content_length > 0)
    {
        // 获取当前还需要接受的正文长度
        size_t remaining_body = content_length - this->_request.body.size();
        if (buffer->GetReadableSize() >= remaining_body)
        {
            // 有足够数据接受完整正文
            this->_request.body += buffer->Read(remaining_body);
        }
        else
        {
            // 数据不足,全部接受现有数据,继续等待剩余数据
            this->_request.body += buffer->Read(buffer->GetReadableSize());
            this->_accept_status = HttpAcceptStatus::ACCEPTING_BODY;
            return true;
        }
    }
    // 完成解析
    this->_accept_status = HttpAcceptStatus::ACCEPTED;
    return true;
}

void HttpContext::ParseRequest(Buffer *buffer)
{
    switch (this->_accept_status)
    {
            // 解析请求行
        case HttpAcceptStatus::ACCEPTING_REQUEST_LINE:
            this->_ParseRequestLine(buffer);
        case HttpAcceptStatus::ACCEPTING_HEADERS:
            // 解析请求头部
            this->_ParseHeaders(buffer);
        case HttpAcceptStatus::ACCEPTING_BODY:
            // 解析请求正文
            this->_ParseBody(buffer);
    }
}

void HttpContext::Reset()
{
    this->_response_status = 200;
    this->_accept_status = HttpAcceptStatus::ACCEPTING_REQUEST_LINE;
    this->_request.Clear();
}
