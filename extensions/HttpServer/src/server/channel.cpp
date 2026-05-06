#include "server/event_loop.h"
// 构造函数
Channel::Channel(EventLoop *event_loop, Socket &&sock)
    : _event_loop(event_loop), _sock(std::move(sock)), _events(0), _revents(0)
{
}

// Channel::Channel(Poller *poller, Socket &&sock) : _poller(poller), _sock(std::move(sock)), _events(0), _revents(0) {}

// 判断当前是否监控可读
bool Channel::ReadAble() { return this->_events & EPOLLIN; }

// 判断当前是否可监控可写
bool Channel::WriteAble() { return this->_events & EPOLLOUT; }

// 启动可读事件监控
void Channel::EnableRead()
{
    this->_events |= EPOLLIN;
    this->AddToPoller();
}

// 启动可写事件监控
void Channel::EnableWrite()
{
    this->_events |= EPOLLOUT;
    this->AddToPoller();
}

// 关闭可读事件监控
void Channel::DisableRead()
{
    this->_events &= ~EPOLLIN;
    this->AddToPoller();
}

// 关闭可写事件监控
void Channel::DisableWrite()
{
    this->_events &= ~EPOLLOUT;
    this->AddToPoller();
}

// 关闭所有事件监控
void Channel::DisableAll()
{
    this->_events = 0;
    this->AddToPoller();
}

// 将当前Channel添加到EventLoop的监控列表中,如果已经存在则更新事件
void Channel::AddToPoller() { this->_event_loop->AddChannel(this); }

// 从EventLoop的监控列表中移除当前Channel
void Channel::Remove()
{
    this->DisableAll();
    this->_event_loop->RemoveChannel(this);
}

// 设置就绪事件
void Channel::SetRevents(uint32_t revents) { this->_revents = revents; }

// 获取文件描述符类型
Socket &Channel::GetSocket() { return this->_sock; }

// 事件处理
/***
 * 先判断错误和断开事件(EPOLLERR、EPOLLHUP)
 * 一旦触发,优先处理并关闭 fd,后续事件就不再处理.
 * 提前return,保证只处理一次
 */
void Channel::HandleEvent()
{
    // 注意: 某些回调(例如 close/error/read/write)内部可能会 Remove + delete Channel。
    // 因此一旦执行这类回调,必须立刻返回,避免后续再次访问 this 导致悬空指针。
    if (this->_revents & EPOLLERR)
    {
        if (this->errorAction) this->errorAction();
        return;
    }

    if (this->_revents & EPOLLHUP)
    {
        if (this->closeAction) this->closeAction();
        return;
    }

    if ((this->_revents & EPOLLIN) || (this->_revents & EPOLLRDHUP) || (this->_revents & EPOLLPRI))
    {
        if (this->readAction) this->readAction();
        return;
    }

    if (this->_revents & EPOLLOUT)
    {
        if (this->writeAction) this->writeAction();
        return;
    }
    if (this->eventAction)
    {
        this->eventAction();
    }
}

// 获取当前关注的事件
uint32_t Channel::GetEvents() const { return this->_events; }