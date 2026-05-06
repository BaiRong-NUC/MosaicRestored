#include <protocol/http/http_server.h>
#include <curl/curl.h>
#include <iostream>
#include <stdexcept>

int main(int argc, char const *argv[])
{
    SetLogLevel(WARNING);
    // 获取计算机CPU核心数量,作为从属线程数量
    int thread_num = std::thread::hardware_concurrency();
    HttpServer server("./wwwroot", 8090, DEFAULT_INACTIVE_TIMEOUT, thread_num);
    server.Listen();
}
