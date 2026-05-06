#include "server/connection.h"

// Start channel 管理的套接字事件回调函数实现
void Connection::_HandleRead()
{
    // 当数组满了等下次再读.
    char buffer[BUFFER_DEFAULT_SIZE] = {0};
    int ret = this->_channel.GetSocket().Recv(buffer, BUFFER_DEFAULT_SIZE, MSG_DONTWAIT);
    if (ret == -2)
    {
        // 对端已优雅关闭：记录为 INFO 并关闭连接，但不当作频繁的 WARNING
        LOG(INFO, "Peer closed connection, closing locally");
        this->Close();
        return;
    }
    else if (ret < 0)
    {
        LOG(WARNING, "Failed to read from socket, connection will be closed");
        // 查看缓冲区是否有数据再决定删除
        this->Close();
        return;
    }
    else if (ret == 0)
    {
        // 暂时没有数据可读（非阻塞或被中断），不视为错误，不打印警告
        return;
    }
    else
    {
        // 将读取到的数据写入输入缓冲区
        this->_in_buffer.Write(buffer, ret);

        // 调用业务处理回调函数
        if (this->message_callback && this->_in_buffer.GetReadableSize() > 0)
        {
            this->message_callback(shared_from_this(), &this->_in_buffer);
        }
    }
}

void Connection::_HandleWrite()
{
    if (this->_out_buffer.GetReadableSize() == 0)
    {
        // 没有数据需要发送,关闭可写事件监控,否则对端缓冲区有空间会频繁触发写事件,但是没有数据可写
        this->_channel.DisableWrite();
        if (this->_state == ConnectState::DISCONNECTING)
        {
            // 连接处于待关闭状态且没有数据需要发送,可以真正关闭连接了
            this->_Release();
        }
        return;
    }

    std::string pending = this->_out_buffer.Read(this->_out_buffer.GetReadableSize());
    int ret = this->_channel.GetSocket().Send(pending.data(), pending.size(), MSG_DONTWAIT);
    if (ret < 0)
    {
        LOG(WARNING, "Failed to write to socket, connection will be closed");
        // 处理缓冲区数据
        if (this->_in_buffer.GetReadableSize() > 0)
        {
            // 连接异常,但输入缓冲区还有数据未处理,调用业务处理回调函数处理剩余数据
            if (this->message_callback)
            {
                this->message_callback(shared_from_this(), &this->_in_buffer);
            }
        }
        // this->Close();
        // 没有数据,可以直接真正删除
        this->_Release();
        return;
    }
    else if (ret == 0)
    {
        this->_out_buffer.Write(pending);
        return;
    }

    if (static_cast<size_t>(ret) < pending.size())
    {
        this->_out_buffer.Write(pending.data() + ret, pending.size() - static_cast<size_t>(ret));
    }
}

void Connection::_HandleClose()
{
    // 处理缓冲区数据
    if (this->_in_buffer.GetReadableSize() > 0)
    {
        // 输入缓冲区还有数据未处理,调用业务处理回调函数处理剩余数据
        if (this->message_callback)
        {
            this->message_callback(shared_from_this(), &this->_in_buffer);
        }
    }
    // 没有数据,可以直接真正删除
    this->_Release();
}

void Connection::_HandleError()
{
    LOG(ERROR, "Connection error occurred, connection will be closed");
    this->_HandleClose();
}

void Connection::_HandleEvent()
{
    // 刷新活跃度,调用组件使用者的任意事件
    if (this->_inactive_release == true)
    {
        // 刷新连接度活跃度,默认延时添加定时器任务时设置的时间,也可以自己指定
        this->_event_loop->RefreshTimerTask(this->_id);
    }

    // 调用组件使用者的任意事件回调函数
    if (this->event_callback)
    {
        this->event_callback(shared_from_this());
    }
}
// End channel 管理的套接字事件回调函数实现

// 连接建立完成,修改连接状态,启动连接读事件监控,连接就绪初始化
void Connection::Established()
{
    this->_event_loop->RunTask(
        [this]()
        {
            assert(this->_state == ConnectState::CONNECTING);
            this->_state = ConnectState::CONNECTED;
            this->_channel.EnableRead();

            // 用户设置的连接建立回调
            if (this->connected_callback)
            {
                this->connected_callback(shared_from_this());
            }
        });
}

// 释放链接
void Connection::_Release()
{
    if (this->_state == ConnectState::DISCONNECTED) return;

    this->_event_loop->RunTask(
        [this]()
        {
            if (this->_state == ConnectState::DISCONNECTED) return;

            // 修改连接状态
            this->_state = ConnectState::DISCONNECTED;

            // 移除事件监控
            this->_channel.Remove();  // 从EventLoop的监控列表中移除当前Channel

            // 关闭描述符
            this->_channel.GetSocket().Close();

            // 取消定时器任务,因为_Release()可能重复调用,这里需要判定timer id是否存在
            if (this->_inactive_release == true)
            {
                this->_event_loop->CancelTimerTask(this->_id);
            }

            // 用户设置的连接关闭回调
            if (this->closed_callback)
            {
                this->closed_callback(shared_from_this());
            }

            // 移除服务器内部对连接的管理,从连接列表中移除连接对象,必须先调用用户设置的函数
            if (this->_server_closed_callback)
            {
                this->_server_closed_callback(shared_from_this());
            }
        });
}

// message使用临时变量保存,防止Send函数来不及执行外界message变量销毁
void Connection::Send(const std::string &message)
{
    this->_event_loop->RunTask(
        [this, message]()
        {
            if (this->_state != ConnectState::CONNECTED)
            {
                LOG(WARNING, "Connection is not in connected state, cannot send message");
                return;
            }

            // 将消息写入输出缓冲区
            this->_out_buffer.Write(message);

            // 启动可写事件监控,当socket可写时会调用_HandleWrite将输出缓冲区的数据发送到socket
            if (this->_channel.WriteAble() == false)  // 避免重复启动可写事件监控
            {
                // 没监控过可写事件
                this->_channel.EnableWrite();
            }
        });
}

void Connection::Close()
{
    this->_event_loop->RunTask(
        [&]()
        {
            if (this->_state == ConnectState::DISCONNECTED || this->_state == ConnectState::DISCONNECTING) return;

            // 修改连接状态
            this->_state = ConnectState::DISCONNECTING;

            // 检查输入缓冲区
            if (this->_in_buffer.GetReadableSize() > 0)
            {
                if (this->message_callback != nullptr) this->message_callback(shared_from_this(), &this->_in_buffer);
            }

            // 检查输出缓冲区
            if (this->_out_buffer.GetReadableSize() > 0)
            {
                // 启动写事件监控,调用_HandleWrite发送数据,当发送数据失败会调用_Release()
                if (this->_channel.WriteAble() == false)
                {
                    // 当写事件启动时会一直尝试发送输出缓冲区,_HandleWrite处理完数据后关闭写事件监控
                    // 当连接为DISCONNECTING状态时,如果输出缓冲区没有数据了就直接调用_Release()真正关闭连接
                    this->_channel.EnableWrite();
                }
            }

            if (this->_out_buffer.GetReadableSize() == 0)
            {
                // 没有数据需要发送,可以直接真正删除了
                this->_Release();
            }
        });
}

void Connection::SetInactiveRelease(bool enable, int timeout)
{
    this->_event_loop->RunTask(
        [this, enable, timeout]()
        {
            this->_inactive_release = enable;
            if (enable)
            {
                // 添加定时器任务,当连接不活跃时自动释放连接,默认10s后到期,也可以自己指定
                if (this->_event_loop->FindTimerTask(this->_id) == false)  // 避免重复添加定时器任务
                {
                    this->_event_loop->AddTimerTask(this->_id, timeout, std::bind(&Connection::_Release, this));
                }
                else
                {
                    // 已经添加过定时器任务了,刷新定时器任务的到期时间
                    this->_event_loop->RefreshTimerTask(this->_id, timeout);
                }
            }
            else
            {
                // 取消定时器任务
                this->_event_loop->CancelTimerTask(this->_id);
            }
        });
}

// 必须立即执行,否则可能存在切换协议后还没生效就触发事件的情况,导致事件处理函数调用错误
// 所以这个函数必须在EventLoop线程上执行,不能切换到其他线程上执行,否则就会进入EventLoop队列
void Connection::SwitchProtocol(const Any &new_context, const Action &connected_callback, const Action &closed_callback,
                                const Action &event_callback, const MessageAction &message_callback)
{
    if (this->_event_loop->InLoop() == false)
    {
        LOG(ERROR, "SwitchProtocol must be called in EventLoop thread");
        exit(EXIT_FAILURE);
    }

    // 这里可以用&,因为这里不存在异步调用的情况,所以变量的生命周期不会在函数结束后销毁导致回调函数空
    this->_event_loop->RunTask(
        [&]()
        {
            // 切换协议,清除上下文,修改处理函数
            this->_context = new_context;
            this->connected_callback = connected_callback;
            this->closed_callback = closed_callback;
            this->event_callback = event_callback;
            this->message_callback = message_callback;
        });
}

Connection::Connection(EventLoop *event_loop, uint64_t id, Socket &&sock)
    : _id(id),
      _state(ConnectState::CONNECTING),
      _inactive_release(false),
      _event_loop(event_loop),
      _channel(event_loop, std::move(sock))
{
    // 设置套接字回调执行函数
    this->_channel.readAction = std::bind(&Connection::_HandleRead, this);
    this->_channel.writeAction = std::bind(&Connection::_HandleWrite, this);
    this->_channel.errorAction = std::bind(&Connection::_HandleError, this);
    this->_channel.closeAction = std::bind(&Connection::_HandleClose, this);
    this->_channel.eventAction = std::bind(&Connection::_HandleEvent, this);

    // 构造函数不能启动channel读事件监控,因为启动后可能立即触发读事件,如果此时存在定时器,会刷新活跃度.
    // 但是此时还没添加定时器,所以启动读时间监控是在设置活跃度销毁函数之后
}

Connection::~Connection()
{
    // 连接对象被销毁时,如果连接还没有真正关闭,先调用_Release()释放连接
    if (this->_state != ConnectState::DISCONNECTED)
    {
        this->_Release();
    }

    LOG(INFO, "Connection object destroyed, id: " << this->_id << "\n\trelease connection: " << this
                                                  << ", Loop thread Id: " << this->GetLoopThreadId() << "\n\n");
}

int Connection::GetSocketFd() { return this->_channel.GetSocket().GetSocketFd(); }

Socket &Connection::GetSocket() { return this->_channel.GetSocket(); }

int Connection::GetConnectionId() const { return this->_id; }

bool Connection::IsConnected() const { return this->_state == ConnectState::CONNECTED; }

ConnectState Connection::GetState() const { return this->_state; }

Any &Connection::GetContext() { return this->_context; }

void Connection::SetContext(const Any &context)
{
    if (this->_event_loop->InLoop() == false)
    {
        LOG(ERROR, "SetContext must be called in EventLoop thread");
        exit(EXIT_FAILURE);
    }
    this->_event_loop->RunTask([&]() { this->_context = context; });
}

std::thread::id Connection::GetLoopThreadId() const { return this->_event_loop->GetThreadId(); }
