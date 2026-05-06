#pragma once
#include "utils/public.h"
#include "server/socket.h"
#include "server/channel.h"
#include "utils/buffer.h"
#include "server/any.h"
#include "server/event_loop.h"
// 对连接管理,封装关于连接的所有操作(为新连接创建Connection对象)

// 连接状态枚举
enum class ConnectState
{
    DISCONNECTED,  // 连接已关闭
    CONNECTING,    // 连接刚建立,待处理
    CONNECTED,     // 连接已建立,通信状态
    DISCONNECTING  // 待关闭状态
};
// 所有对连接的操作必须在同一个线程
class Connection : public std::enable_shared_from_this<Connection>
{
   private:
    uint64_t _id;  // 连接ID,可以是文件描述符或者其他唯一标识符
    // Socket _sock;        // 连接的套接字对象,socket被_channel右值引用拿走了
    Channel _channel;     // 连接的Channel对象,用于监控连接的事件
    Buffer _in_buffer;    // 输入缓冲区,保存从socket中读取的数据
    Buffer _out_buffer;   // 输出缓冲区,保存要发送到socket的数据
    Any _context;         // 连接的上下文,可以保存连接相关的任意数据,如HTTP请求信息等
    ConnectState _state;  // 连接状态,如已连接;正在关闭;已关闭等
    using PtrConnection = std::shared_ptr<Connection>;
    using Action = std::function<void(const PtrConnection &)>;  // 连接事件回调函数类型,参数为当前连接的智能指针
    bool _inactive_release;                                     // 是否启用连接不活跃时自动释放连接的机制
    // uint64_t _timer_id = this->_id;                         // 连接的定时器ID,用于管理连接的定时器,如心跳检测等
    using MessageAction = std::function<void(const PtrConnection &, Buffer *)>;  // 业务处理函数
    EventLoop *_event_loop;                                                      // 关联的EventLoop对象
    void _Release();                                                             // 释放连接

    // channel模块的回调函数
    void _HandleRead();   // 可读事件回调函数,从socket读取数据到输入缓冲区,并调用业务处理函数
    void _HandleWrite();  // 可写事件回调函数,将输出缓冲区的数据发送到socket
    void _HandleClose();  // 连接关闭事件回调函数,处理连接关闭的相关逻辑
    void _HandleError();  // 错误事件回调函数,处理连接错误的相关逻辑
    void _HandleEvent();  // 任意事件回调函数,处理连接的任意事件,如刷新连接活跃度等
   public:
    // 用户提供
    Action connected_callback;       // 连接建立回调函数
    Action closed_callback;          // 连接关闭回调函数
    Action event_callback;           // 连接事件回调函数,如刷新连接活跃度
    MessageAction message_callback;  // 业务处理回调
    Action _server_closed_callback;  // 服务器主动关闭连接的回调函数

    Connection(EventLoop *event_loop, uint64_t id, Socket &&sock);  // 构造函数,参数为连接ID和套接字对象
    ~Connection();

    void Send(const std::string &message);                   // 发送消息到连接
    void Close();                                            // 关闭连接
    void SetInactiveRelease(bool enable, int timeout = 10);  // 设置连接不活跃时自动释放连接的机制,以s为单位
    void SwitchProtocol(const Any &new_context, const Action &connected_callback, const Action &closed_callback,
                        const Action &event_callback,
                        const MessageAction &message_callback);  // 切换协议,清除上下文,修改处理函数

    int GetSocketFd();              // 获取连接的文件描述符
    Socket &GetSocket();            // 获取连接的套接字对象
    int GetConnectionId() const;    // 获取连接ID
    ConnectState GetState() const;  // 获取连接状态
    bool IsConnected() const;       // 判断连接是否处于已连接状态
    Any &GetContext();  // 获取连接的上下文,连接建立完成后可以通过上下文保存连接相关的任意数据,如HTTP请求信息等
    void SetContext(const Any &context);  // 设置连接的上下文
    void Established();                   // 连接就绪初始化
    std::thread::id GetLoopThreadId() const;
};