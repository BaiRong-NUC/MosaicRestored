#pragma once
#include <utils/public.h>
#include <utils/utils.h>
#include <protocol/http/http_context.h>
#include <server/tcp_server.h>
#include <protocol/http/html/http_error_html.h>

/**
 * 功能:
 * 1. GET 請求處理函數的映射表,string是正則表達式
 * 2. POST 請求的路由映射表
 * 3. PUT 請求的路由映射表
 * 4. DELETE 請求的路由映射表
 * 5. 靜態資源相對根目錄
 * 6. TCP 服務器進行鏈接IO操作
 * 流程:
 * 1. 從socket接收HTTP請求數據
 * 2. 調用OnMessage回調函數處理請求數據
 * 3. 對請求進行解析,得到HttpRequest,HttpResponse對象
 * 4. 請求路由查找
 *      - 靜態資源請求: 根據URI查找對應的文件,讀取文件內容作為響應正文,設置Content-Type等響應頭部字段
 *      - 功能請求: 根據URI查找對應的處理函數,調用函數處理請求,得到響應內容和狀態碼,設置HttpResponse對象
 * 5. 將HttpResponse對象組織成http格式進行發送
 */

#define DEFAULT_INACTIVE_TIMEOUT 30 // 默認非活躍連接超時時間,以s為單位,超時後自動關閉不活躍的連接

class HttpServer
{
private:
    using HandlerFunc = std::function<void(const HttpRequest &, HttpResponse &)>;
    struct RouteHandler
    {
        std::regex pattern;
        HandlerFunc handler;
    };
    // 方法到路由映射表的映射表,方便根据方法查找对应的路由映射表
    std::unordered_map<std::string, std::vector<RouteHandler>> _method_handlers;
    TcpServer _tcp_server;                                      // TCP服務器
    void _OnMessage(const PtrConnection &conn, Buffer *buffer); // 處理請求數據的回調函數
    void _OnConnected(const PtrConnection &conn);               // 設置tcp上下文
    // 處理請求的函數,根據請求路由查找對應的處理函數,調用函數處理請求,得到響應內容和狀態碼,設置HttpResponse對象
    void _HandleRequest(const PtrConnection &conn, HttpRequest &request, HttpResponse &response);
    // 查找处理函数
    HandlerFunc _FindHandler(const std::string &method, const std::string &uri, int &status_code);

    // 判断请求是否是静态资源请求
    bool _IsStaticResource(HttpRequest &request);

    // 简单选择默认错误响应内容
    const HttpResponse &_GetErrorResponse(int status_code);

public:
    const std::string static_root; // 靜態資源根目錄
    // 構造函數,timeout<=0則不啟用自動關閉非活躍連接的機制
    HttpServer(const std::string &root, uint16_t port, int timeout = DEFAULT_INACTIVE_TIMEOUT, int thread_num = 0,
               bool reseAddr = true, bool noBlock = true, const std::string &ip = "0.0.0.0",
               int business_thread_num = 0);
    // uri_pattern是正則表達式,用於匹配請求URI,handler是處理函數,接受HttpRequest對象和HttpResponse對象參數,用於處理請求並設置響應內容
    void Get(const std::string &uri_pattern, HandlerFunc handler);
    void Post(const std::string &uri_pattern, HandlerFunc handler);
    void Put(const std::string &uri_pattern, HandlerFunc handler);
    void Delete(const std::string &uri_pattern, HandlerFunc handler);

    // 錯誤默认響應
    HttpResponse response_404; // 默認錯誤響應,用戶可以修改默認響應的內容,如狀態碼、響應頭部和消息體等
    HttpResponse response_405;
    HttpResponse response_error;

    // 設置超時時間,以s為單位,超時後自動關閉不活躍的連接,如果timeout <= 0則不啟用自動關閉非活躍連接的機制
    void SetInactiveTimeout(int timeout);

    // 啟動服務器,開始接受和處理請求
    void Listen();

    // 構造并發送HTTP響應
    void SendResponse(const PtrConnection &conn, const HttpRequest &client_request, HttpResponse &server_response);
};