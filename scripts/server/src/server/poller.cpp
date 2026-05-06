#include "server/poller.h"
#include "server/channel.h"

// 构造函数
Poller::Poller()
{
    // epoll_create参数仅用于指定内核预分配的事件数量,实际监控的文件描述符数量不受限制
    this->_epollfd = epoll_create(MAX_EPOLL_EVENTS);
    if (this->_epollfd == -1)
    {
        LOG(ERROR, "Failed to create epoll instance");
        exit(EXIT_FAILURE); // 立即退出程序，返回失败状态码
    }
}
// 析构函数
Poller::~Poller()
{
    if (this->_epollfd != -1)
    {
        close(this->_epollfd);
    }
}

// 更新epoll事件的内部函数
bool Poller::_UpdateEpoll(Channel *channel, uint32_t op)
{
    int fd = channel->GetSocket().GetSocketFd();
    struct epoll_event ev;
    ev.data.fd = fd;                  // 将文件描述符存储在事件数据中
    ev.events = channel->GetEvents(); // 获取当前关注的事件
    int ret = epoll_ctl(this->_epollfd, op, fd, &ev);
    return ret == 0;
}

// 查找_channels中是否存在sock
bool Poller::_FindSocket(const Socket &sock)
{
    return this->_channels.find(sock.GetSocketFd()) != this->_channels.end();
}
bool Poller::_FindSocket(int fd)
{
    return this->_channels.find(fd) != this->_channels.end();
}

// 添加/更新Channel到监控列表
void Poller::AddChannel(Channel *channel)
{
    const Socket &sock = channel->GetSocket();
    int fd = sock.GetSocketFd();
    if (!this->_FindSocket(sock))
    {
        // 新增Channel
        this->_channels[fd] = channel;
        if (!this->_UpdateEpoll(channel, EPOLL_CTL_ADD))
        {
            LOG(WARNING, "Failed to add channel to epoll");
        }
    }
    else
    {
        // 更新Channel
        if (!this->_UpdateEpoll(channel, EPOLL_CTL_MOD))
        {
            LOG(WARNING, "Failed to modify channel in epoll");
        }
    }
}

// 从监控列表移除Channel
void Poller::RemoveChannel(Channel *channel)
{
    const Socket &sock = channel->GetSocket();
    int fd = sock.GetSocketFd();
    if (this->_FindSocket(sock))
    {
        this->_channels.erase(fd);
        if (!this->_UpdateEpoll(channel, EPOLL_CTL_DEL))
        {
            LOG(WARNING, "Failed to remove channel from epoll");
        }
    }
}

// 获取活跃事件的Channel列表
std::vector<Channel *> Poller::Poll(int timeout)
{
    int nfds = epoll_wait(this->_epollfd, this->_events, MAX_EPOLL_EVENTS, timeout);
    std::vector<Channel *> activeChannels;
    if (nfds < 0)
    {
        if (errno == EINTR)
        {
            LOG(WARNING, "epoll_wait interrupted by signal");
            return activeChannels;
        }
        LOG(ERROR, "Failed to wait for epoll events: \n\t" << strerror(errno));
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < nfds; ++i)
    {
        int fd = this->_events[i].data.fd;
        if (this->_FindSocket(fd) == false)
        {
            LOG(ERROR, "Received event for unknown file descriptor: " << fd);
            continue;
        }
        Channel *channel = this->_channels[fd];
        channel->SetRevents(this->_events[i].events); // 设置就绪事件
        activeChannels.push_back(channel);
    }
    return activeChannels;
}