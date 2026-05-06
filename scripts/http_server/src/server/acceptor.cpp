#include "server/acceptor.h"

Acceptor::Acceptor(EventLoop *event_loop, uint16_t port, bool reseAddr, bool noBlock, const std::string &ip)
    : _event_loop(event_loop)
{
    Socket listen_sock;
    listen_sock.CreateServer(port, reseAddr, noBlock, ip);  // 创建监听套接字
    this->_listen_channel =
        std::make_shared<Channel>(event_loop, std::move(listen_sock));  // 创建监听套接字的Channel对象

    this->_listen_channel->readAction = std::bind(&Acceptor::_HandleRead, this);  // 设置监听套接字的可读事件回调函数
    // this->_listen_channel->EnableRead();                                          // 启动监听套接字的
    /**
     * 不能将启动读事件监控放到构造函数
     * 可能在启动监控后立刻有事件就绪,但是此时new_connection_callback还为空,此时链接什么都没做
     */
}

void Acceptor::_HandleRead()
{
    std::string clientIp;
    uint16_t clientPort;
    Socket clientSock;
    if (this->_listen_channel->GetSocket().Accept(clientIp, clientPort, clientSock))
    {
        LOG(INFO, "New connection from " + clientIp + ":" + std::to_string(clientPort));
        if (this->new_connection_callback)
        {
            // 调用新连接事件回调函数,将新连接的套接字对象作为参数传递
            // clientSock声明周期交给回调函数管理,里面有全局智能指针哈希表,不用担心clientSock
            this->new_connection_callback(std::move(clientSock));
        }
    }
    else
    {
        LOG(ERROR, "Failed to accept new connection");
    }
}