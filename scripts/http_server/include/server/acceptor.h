#pragma once
#include "utils/public.h"
#include "server/socket.h"
#include "server/channel.h"
#include "server/event_loop.h"
// 管理监听套接字的创建,创建完毕后调用设置事件触发函数获取新连接
class Acceptor
{
   private:
    // Socket _listen_sock;  // 监听套接字对象,负责监听新连接
    std::shared_ptr<Channel> _listen_channel;       // 监听套接字的Channel,用于监控监听套接字的事件
    EventLoop *_event_loop;                         // 关联的EventLoop对象,用于在新连接到来时执行回调函数
    using Action = std::function<void(Socket &&)>;  // 新连接事件回调函数类型,参数为新连接的套接字对象
    // channel模块监控函数
    void _HandleRead();  // 可读事件回调函数,当监听套接字可读时说明有新连接到来,从监听套接字接受新连接并调用回调函数
   public:
    Action new_connection_callback;  // 新连接事件回调函数
    Acceptor(EventLoop *event_loop, uint16_t port, bool reseAddr=true, bool noBlock=true,
             const std::string &ip="0.0.0.0");  // 构造函数,参数为监听端口和关联的EventLoop对象

    void Listen() { this->_listen_channel->EnableRead(); }  // 启动监听套接字的可读事件监控,当可读时说明有新连接到来
};