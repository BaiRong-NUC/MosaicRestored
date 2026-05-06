#include <protocol/http/http_server.h>
#include <curl/curl.h>

#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <iostream>
#include <stdexcept>

namespace
{
    const char *DEFAULT_PYTHON_RESTORE_URL = "http://127.0.0.1:8091/restore";

    struct ProxyResult
    {
        CURLcode curl_code = CURLE_OK;
        long status_code = 0;
        std::string body;
        std::string content_type;
        std::string output_url;
        char error_buffer[CURL_ERROR_SIZE] = {0};
    };

    size_t WriteToString(char *ptr, size_t size, size_t nmemb, void *userdata)
    {
        size_t total_size = size * nmemb;
        auto *data = static_cast<std::string *>(userdata);
        data->append(ptr, total_size);
        return total_size;
    }

    std::string Trim(std::string value)
    {
        auto is_space = [](unsigned char ch)
        { return std::isspace(ch); };
        value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](unsigned char ch)
                                                { return !is_space(ch); }));
        value.erase(std::find_if(value.rbegin(), value.rend(), [&](unsigned char ch)
                                 { return !is_space(ch); })
                        .base(),
                    value.end());
        return value;
    }

    std::string ToLower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch)
                       { return std::tolower(ch); });
        return value;
    }

    size_t CaptureHeader(char *buffer, size_t size, size_t nitems, void *userdata)
    {
        size_t total_size = size * nitems;
        auto *result = static_cast<ProxyResult *>(userdata);
        std::string header_line(buffer, total_size);
        size_t colon_pos = header_line.find(':');
        if (colon_pos == std::string::npos)
        {
            return total_size;
        }

        std::string key = ToLower(Trim(header_line.substr(0, colon_pos)));
        std::string value = Trim(header_line.substr(colon_pos + 1));
        if (key == "x-replicate-output-url")
        {
            result->output_url = value;
        }
        return total_size;
    }

    std::string JsonEscape(const std::string &value)
    {
        std::string escaped;
        escaped.reserve(value.size());
        for (char ch : value)
        {
            switch (ch)
            {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped += ch;
                break;
            }
        }
        return escaped;
    }

    std::string JsonError(const std::string &message)
    {
        return std::string("{\"detail\":\"") + JsonEscape(message) + "\"}";
    }

    std::string PythonRestoreUrl()
    {
        const char *configured_url = std::getenv("MOSAIC_PYTHON_URL");
        if (configured_url && configured_url[0] != '\0')
        {
            return configured_url;
        }
        return DEFAULT_PYTHON_RESTORE_URL;
    }

    void AppendQueryParam(CURL *curl, std::string &url, bool &has_query, const std::string &key, const std::string &value)
    {
        char *escaped_key = curl_easy_escape(curl, key.c_str(), static_cast<int>(key.size()));
        char *escaped_value = curl_easy_escape(curl, value.c_str(), static_cast<int>(value.size()));
        if (!escaped_key || !escaped_value)
        {
            if (escaped_key)
                curl_free(escaped_key);
            if (escaped_value)
                curl_free(escaped_value);
            throw std::runtime_error("Failed to encode query parameter.");
        }

        url += has_query ? '&' : '?';
        url += escaped_key;
        url += '=';
        url += escaped_value;
        has_query = true;

        curl_free(escaped_key);
        curl_free(escaped_value);
    }

    ProxyResult ForwardRestoreRequest(const HttpRequest &request)
    {
        ProxyResult result;
        CURL *curl = curl_easy_init();
        if (!curl)
        {
            result.curl_code = CURLE_FAILED_INIT;
            return result;
        }

        std::string url = PythonRestoreUrl();
        bool has_query = url.find('?') != std::string::npos;
        const std::vector<std::string> forwarded_params = {
            "upscale", "face_upsample", "background_enhance", "codeformer_fidelity", "save"};
        try
        {
            for (const std::string &param : forwarded_params)
            {
                if (request.HasQueryParam(param))
                {
                    AppendQueryParam(curl, url, has_query, param, request.GetQueryParam(param));
                }
            }
        }
        catch (const std::exception &error)
        {
            curl_easy_cleanup(curl);
            result.curl_code = CURLE_URL_MALFORMAT;
            std::string message = error.what();
            std::copy(message.begin(), message.end(), result.error_buffer);
            return result;
        }

        std::string content_type = request.GetHeader("Content-Type");
        if (content_type.empty())
        {
            content_type = "application/octet-stream";
        }

        struct curl_slist *headers = nullptr;
        std::string content_type_header = "Content-Type: " + content_type;
        headers = curl_slist_append(headers, content_type_header.c_str());
        headers = curl_slist_append(headers, "Accept: image/png, application/json");
        headers = curl_slist_append(headers, "Expect:");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.body.data());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(request.body.size()));
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result.body);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, CaptureHeader);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &result);
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, result.error_buffer);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 600L);

        result.curl_code = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.status_code);
        char *upstream_content_type = nullptr;
        curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &upstream_content_type);
        if (upstream_content_type)
        {
            result.content_type = upstream_content_type;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return result;
    }
} // namespace

int main(int argc, char const *argv[])
{
    SetLogLevel(WARNING);
    curl_global_init(CURL_GLOBAL_DEFAULT);
    // 获取计算机CPU核心数量,作为从属线程数量
    int thread_num = std::thread::hardware_concurrency();
    HttpServer server("./wwwroot", 8090, DEFAULT_INACTIVE_TIMEOUT, thread_num);
    server.Post("^/api/restore$", [](const HttpRequest &request, HttpResponse &response)
                {
                    if (request.body.empty())
                    {
                        response.status_code = 400;
                        response.SetBody(JsonError("Request body must contain image bytes."), "application/json");
                        return;
                    }

                    ProxyResult proxy_result = ForwardRestoreRequest(request);
                    if (proxy_result.curl_code != CURLE_OK)
                    {
                        std::string message = proxy_result.error_buffer[0] != '\0'
                                                  ? proxy_result.error_buffer
                                                  : curl_easy_strerror(proxy_result.curl_code);
                        response.status_code = 502;
                        response.SetBody(JsonError("Python restore service request failed: " + message),
                                         "application/json");
                        return;
                    }

                    response.status_code = proxy_result.status_code > 0 ? static_cast<int>(proxy_result.status_code) : 502;
                    std::string content_type = proxy_result.content_type.empty() ? "application/octet-stream" : proxy_result.content_type;
                    response.SetBody(proxy_result.body, content_type);
                    if (!proxy_result.output_url.empty())
                    {
                        response.SetHeader("X-Replicate-Output-Url", proxy_result.output_url);
                    } });
    server.Listen();
    curl_global_cleanup();
}
