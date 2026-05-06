#pragma once
#include "server/socket.h"
#include "utils/public.h"
#include "utils/log.h"
#define MAX_EPOLL_EVENTS 1024

class Channel;

// 通过epoll实现对文件描述符的IO事件监控
class Poller
{
private:
    int _epollfd;                                     // epoll实例的文件描述符
    struct epoll_event _events[MAX_EPOLL_EVENTS];     // 存储epoll事件的数组
    std::unordered_map<int, Channel *> _channels;     // 文件描述符到Channel对象的映射
    bool _UpdateEpoll(Channel *channel, uint32_t op); // 更新epoll事件的内部函数
    bool _FindSocket(const Socket &sock);             // 查找_channels中是否存在sock
    bool _FindSocket(int fd);                         // 查找_channels中是否存在文件描述符
public:
    Poller();
    ~Poller();
    Poller(const Poller &) = delete;
    Poller &operator=(const Poller &) = delete;
    Poller(Poller &&) = delete;
    Poller &operator=(Poller &&) = delete;
    // 添加/更新Channel到监控列表
    void AddChannel(Channel *channel);
    // 从监控列表移除Channel
    void RemoveChannel(Channel *channel);
    // 获取活跃事件的Channel列表,timeout单位为毫秒,返回值为触发事件的Channel列表,默认阻塞等待
    std::vector<Channel *> Poll(int timeout = -1);
};