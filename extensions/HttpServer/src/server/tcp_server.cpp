#include "server/tcp_server.h"

TcpServer::TcpServer(uint16_t port, int thread_num, bool reseAddr, bool noBlock, const std::string &ip,
                     int business_thread_num)
    : port(port),
      thread_num(thread_num),
      _connection_id(0),
      _timer_id(0),
      _acceptor(&_baseloop, port, reseAddr, noBlock, ip),
      _loop_thread_pool(&_baseloop, thread_num),
      _business_thread_pool(business_thread_num < 0 ? 0 : static_cast<size_t>(business_thread_num)),
      _inactive_release(false),
      _inactive_timeout(10)

{
    this->connected_callback = nullptr;
    this->closed_callback = nullptr;
    this->event_callback = nullptr;
    this->message_callback = nullptr;

    // 忽略(SIG_IGN) SIGPIPE
    signal(SIGPIPE, SIG_IGN);
}

TcpServer::~TcpServer() {}

/**
 * TcpServer::SetInactiveRelease只会被调用一次，
 * 所以this->_inactive_release = enable;this->_inactive_timeout线程安全
 * 这里为了可能会运行中多次调用,为了兼容,防止出错使用原子变量,但其实不需要原子变量,直接使用普通成员变量就行了
 */
void TcpServer::SetInactiveRelease(bool enable, int timeout)
{
    this->_inactive_release.store(enable, std::memory_order_relaxed);
    this->_inactive_timeout.store(timeout, std::memory_order_relaxed);
}

void TcpServer::AddTimerTask(TimerAction &action, uint64_t expireTime)
{
    this->_baseloop.AddTimerTask(this->_timer_id++, expireTime, action);
}

void TcpServer::Run()
{
    this->_acceptor.new_connection_callback = [this](Socket &&clientSock)
    {
        EventLoop *loop = this->_loop_thread_pool.GetSubEventLoop(); // 轮询分配EventLoop对象
        PtrConnection clientConnection =
            std::make_shared<Connection>(loop, this->_connection_id++, std::move(clientSock),
                                         &this->_business_thread_pool);
        this->_connections[clientConnection->GetConnectionId()] = clientConnection;

        // 关闭连接
        clientConnection->closed_callback = this->closed_callback;

        // 连接事件
        clientConnection->connected_callback = this->connected_callback;

        // 客户端套接字可读时,业务处理
        clientConnection->message_callback = this->message_callback;

        // 任意事件处理
        clientConnection->event_callback = this->event_callback;

        // 连接删除处理
        clientConnection->_server_closed_callback = [this](const PtrConnection &conn)
        {
            const auto connId = conn->GetConnectionId();
            this->_baseloop.RunTask(
                [this, connId]()
                {
                    // LOG(INFO, "Client disconnected, id: " << conn->GetConnectionId() << "\n\tconnection Address: "
                    //                                       << conn << ", Loop thread Id: " <<
                    //                                       conn->GetLoopThreadId());
                    this->_connections.erase(connId);
                }); // 从连接列表中移除连接对象
        };
        clientConnection->SetInactiveRelease(this->_inactive_release.load(std::memory_order_relaxed),
                                             this->_inactive_timeout.load(std::memory_order_relaxed));
        clientConnection->Established(); // 连接就绪初始化,启动可读监控
    };

    this->_acceptor.Listen(); // 启动监听套接字的可读事件监控,当可读时说明有新连接到来
    this->_baseloop.Start();  // 启动事件循环,监控事件
}
