#include "server/socket.h"

Socket::Socket() : _sockfd(-1) {}

Socket::Socket(int fd) : _sockfd(fd) {}

Socket::Socket(Socket &&other) noexcept : _sockfd(other._sockfd) { other._sockfd = -1; }

Socket &Socket::operator=(Socket &&other) noexcept
{
    if (this != &other)
    {
        this->Close();
        this->_sockfd = other._sockfd;
        other._sockfd = -1;
    }
    return *this;
}

Socket::~Socket() { this->Close(); }

// 创建监听套接字
bool Socket::Create()
{
    // IPV4
    this->_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (this->_sockfd == -1)
    {
        return false;
    }
    return true;
}

// 绑定
bool Socket::Bind(const std::string &ip, uint16_t port)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    socklen_t len = sizeof(struct sockaddr_in);
    int ret = bind(this->_sockfd, (struct sockaddr *)&addr, len);
    if (ret == -1)
    {
        return false;
    }
    return true;
}

// 监听
bool Socket::Listen(int backlog)
{
    int ret = listen(this->_sockfd, backlog);
    if (ret == -1)
    {
        return false;
    }
    return true;
}

// 客户端连接服务器
bool Socket::Connect(const std::string &ip, uint16_t port)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    socklen_t len = sizeof(struct sockaddr_in);
    int ret = connect(this->_sockfd, (struct sockaddr *)&addr, len);
    if (ret == -1)
    {
        return false;
    }
    return true;
}

// 接受客户端连接
bool Socket::Accept(std::string &clientIp, uint16_t &clientPort, Socket &clientSocket)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(struct sockaddr_in);
    int clientFd = accept(this->_sockfd, (struct sockaddr *)&addr, &len);
    if (clientFd == -1)
    {
        return false;
    }
    clientIp = inet_ntoa(addr.sin_addr);
    clientPort = ntohs(addr.sin_port);
    clientSocket = Socket(clientFd);
    return true;
}

// 接受数据
// 返回值约定:
//  >0 : 读取到的字节数
//   0 : 非阻塞/被中断且当前没有数据可读（可重试）
//  -1 : 发生错误
//  -2 : 对端已优雅关闭（recv 返回 0）
ssize_t Socket::Recv(void *buffer, size_t len, int flags)
{
    ssize_t ret = recv(this->_sockfd, buffer, len, flags);
    if (ret == 0)
    {
        // 对端已优雅关闭
        return -2;
    }

    if (ret < 0)
    {
        // EAGAIN/EWOULDBLOCK: 非阻塞套接字没有数据可读
        // EINTR: recv 被信号中断
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        {
            return 0;
        }
        return -1;  // 其他错误
    }
    return ret;
}

// 发送数据
ssize_t Socket::Send(const void *buffer, size_t len, int flags)
{
    int ret = send(this->_sockfd, buffer, len, flags | MSG_NOSIGNAL);
    if (ret <= 0)
    {
        // EANGAIN: 无数据可读
        // EINTR: send被信号中断
        if (errno == EAGAIN || errno == EINTR)
        {
            return 0;
        }
        return -1;  // 其他错误,返回-1表示连接关闭或者发生错误
    }
    return ret;  // 返回实际发送的字节数
}

// 关闭套接字
void Socket::Close()
{
    if (this->_sockfd != -1)
    {
        close(this->_sockfd);
        this->_sockfd = -1;
    }
}

// 设置套接字属性为非阻塞,此时recv,send flags参数为0时也不会阻塞
void Socket::SetNoBlock()
{
    // 获取当前套接字的文件状态标志
    int flags = fcntl(this->_sockfd, F_GETFL, 0);
    if (flags == -1)
    {
        return;
    }
    // 添加非阻塞标志
    fcntl(this->_sockfd, F_SETFL, flags | O_NONBLOCK);
}

// 开启地址复用
void Socket::SetReuseAddr(bool reuse)
{
    // 地址复用
    int optval = reuse ? 1 : 0;
    setsockopt(this->_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    // 端口复用
    optval = reuse ? 1 : 0;
    setsockopt(this->_sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}

// 创建服务器连接,默认绑定所有网卡
bool Socket::CreateServer(uint16_t port, bool reuseAddr, bool noBlock, const std::string &ip)
{
    // 1. 创建套接字
    if (!this->Create())
    {
        LOG(ERROR, "Failed to create listen socket");
        return false;
    }
    // 2. 开启地址复用(必须在 Bind 之前设置，否则无效)
    if (reuseAddr)
    {
        this->SetReuseAddr(true);
    }

    // 3. 绑定IP和端口
    if (!this->Bind(ip, port))
    {
        LOG(ERROR, "Failed to bind socket to " + ip + ":" + std::to_string(port));
        exit(EXIT_FAILURE);
        return false;
    }
    // 4. 监听
    if (!this->Listen())
    {
        LOG(ERROR, "Failed to listen on socket");
        exit(EXIT_FAILURE);
        return false;
    }
    // 5. 设置非阻塞
    if (noBlock == true)
    {
        this->SetNoBlock();
    }
    return true;
}

// 创建客户端连接
bool Socket::CreateClient(const std::string &ip, uint16_t port)
{
    // 1. 创建套接字
    if (!this->Create())
    {
        LOG(ERROR, "Failed to create client socket");
        return false;
    }
    // 2. 连接服务器
    if (!this->Connect(ip, port))
    {
        LOG(ERROR, "Failed to connect to server " + ip + ":" + std::to_string(port));
        return false;
    }
    return true;
}