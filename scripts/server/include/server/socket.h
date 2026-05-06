#include "utils/public.h"
#include "utils/log.h"
#pragma once
#define MAX_LISTEN_QUEUE 5
class Socket
{
private:
    int _sockfd; // 套接字文件描述符
public:
    Socket();
    Socket(int fd);
    Socket(const Socket &) = delete;
    Socket &operator=(const Socket &) = delete;
    Socket(Socket &&other) noexcept;
    Socket &operator=(Socket &&other) noexcept;
    ~Socket();
    // 创建监听套接字
    bool Create();
    // 绑定IP和端口
    bool Bind(const std::string &ip, uint16_t port);
    // 监听
    bool Listen(int backlog = MAX_LISTEN_QUEUE);
    // 客户端向服务器发起链接
    bool Connect(const std::string &ip, uint16_t port);
    // 接受客户端连接,通过输出参数返回Socket对象
    bool Accept(std::string &clientIp, uint16_t &clientPort, Socket &clientSocket);
    // 接受数据,MSG_DONTWAIT:非阻塞接收
    ssize_t Recv(void *buffer, size_t len, int flags = 0); // 默认阻塞
    // 发送数据
    ssize_t Send(const void *buffer, size_t len, int flags = 0); // 默认阻塞
    // 创建服务器连接,默认绑定所有网卡
    bool CreateServer(uint16_t port, bool reuseAddr = true, bool noBlock = true, const std::string &ip = "0.0.0.0");
    // 创建客户端连接
    bool CreateClient(const std::string &ip, uint16_t port);
    // 开启地址复用
    void SetReuseAddr(bool reuse = true);
    // 设置套接字属性为非阻塞
    void SetNoBlock();
    // 关闭套接字
    void Close();

    // 获取文件描述符
    int GetSocketFd() const { return this->_sockfd; }
};