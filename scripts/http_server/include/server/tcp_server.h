#pragma once
#include "utils/public.h"
#include "server/acceptor.h"
#include "server/loop_thread_pool.h"
#include "server/connection.h"
using PtrConnection = std::shared_ptr<Connection>;
using Action = std::function<void(const PtrConnection &)>;
using MessageAction = std::function<void(const PtrConnection &, Buffer *)>;  // 业务处理函数
class TcpServer
{
   private:
    uint64_t _connection_id;  // 连接ID,每当有新连接到来时递增,用于唯一标识连接
    uint64_t _timer_id;       // 定时器ID,每当有新连接到来时递增,用于唯一标识定时器
    EventLoop _baseloop;      // EventLoop对象,实现对监听套接字的管理
    Acceptor _acceptor;       // Acceptor对象,创建一个监听套接字
    // std::unordered_map<int, PtrConnection>对象,实现对多个Connection对象的管理
    std::unordered_map<int, PtrConnection> _connections;
    LoopThreadPool _loop_thread_pool;     // LoopThreadPool对象,创建loop线程池,对连接进行事件监控及处理
    std::atomic<bool> _inactive_release;  // 是否启用连接不活跃时自动释放连接的机制
    std::atomic<int> _inactive_timeout;   // 连接不活跃时自动释放连接的超时时间,以s为单位
   public:
    const int thread_num;  // 从属线程数量
    const uint16_t port;   // 监听端口
                           // 构造函数,参数为监听端口和从属线程数量
    TcpServer(uint16_t port, int thread_num = 0, bool reseAddr = true, bool noBlock = true,
              const std::string &ip = "0.0.0.0");
    ~TcpServer();
    // 用户提供
    Action connected_callback;       // 连接建立回调函数
    Action closed_callback;          // 连接关闭回调函数
    Action event_callback;           // 连接事件回调函数,如刷新连接活跃度
    MessageAction message_callback;  // 业务处理回调
    // 设置连接不活跃时自动释放连接的机制,以s为单位
    void SetInactiveRelease(bool enable, int timeout = 10);
    // 定时器任务
    using TimerAction = std::function<void()>;
    void AddTimerTask(TimerAction &action, uint64_t expireTime);

    void Run();
};

/**
 * TcpServer模块: 对所有模块的整合,通过TcpServer类对外提供接口,对外隐藏所有模块的细节
 * 管理:
 *  1. Acceptor对象,创建一个监听套接字
 *  2. EventLoop对象,实现对监听套接字的管理
 *  3. std::unordered_map<int, PtrConnection> connections;对象,实现对多个Connection对象的管理
 *  4. LoopThreadPool对象,创建loop线程池,对连接进行事件监控及处理
 * 功能:
 *  1. 设置从属线程
 *  2. 启动服务器
 *  3. 设置回调函数
 *  4. 启动非活跃销毁
 *  5. 添加定时任务
 * 流程:
 *  - TcpServer实例化Acceptor+EventLoop
 *  - Acceptor被挂到baseloop上,监听套接字被挂到baseloop上,监控新连接到来
 *  - 创建新连接Connection,设置功能回调,启用非活跃超时销毁.
 *  - 将新连接分配给LoopThreadPool中的一个EventLoop,由该EventLoop监控新连接的事件,并调用相应的回调函数处理事件
 */