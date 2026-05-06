#pragma once
#include "utils/public.h"
#include "server/socket.h"

class Poller;
class EventLoop;

// 对文件描述符的事件监控
class Channel
{
private:
    Socket _sock;                         // 监控的文件描述符
    uint32_t _events;                     // 关注的事件
    uint32_t _revents;                    // 当前触发的事件
    using Action = std::function<void()>; // 事件回调函数类型
    // [[deprecated("请使用 EventLoop 管理 Channel")]]
    // Poller* _poller;         // 事件监控器,已废弃,请使用EventLoop 管理Channel
    EventLoop *_event_loop; // 关联的EventLoop对象
public:
    Channel(EventLoop *event_loop, Socket &&sock);

    // [[deprecated("请使用 EventLoop 管理 Channel")]]
    // Channel(Poller *poller, Socket &&sock); // 已废弃,请使用EventLoop 管理Channel
    // 可读事件回调函数
    Action readAction;
    // 可写事件回调函数
    Action writeAction;
    // 错误事件回调函数
    Action errorAction;
    // 连接断开事件回调函数
    Action closeAction;
    // 任意事件触发回调函数
    Action eventAction;
    // 当前是否监控可读
    bool ReadAble();
    // 当前是否监控可写
    bool WriteAble();
    // 启动可读事件监控
    void EnableRead();
    // 启动可写事件监控
    void EnableWrite();
    // 关闭可读事件监控
    void DisableRead();
    // 关闭可写事件监控
    void DisableWrite();

    // 关闭所有事件监控
    void DisableAll();

    // 将当前Channel添加到Poller的监控列表中,如果已经存在则更新事件
    void AddToPoller();
    // 从Poller的监控列表中移除当前Channel
    void Remove();

    // 事件处理
    void HandleEvent(); // 根据当前触发的事件调用对应的回调函数

    // 获取文件描述符
    Socket &GetSocket();

    // 设置就绪事件
    void SetRevents(uint32_t revents);

    // 获取当前关注的事件
    uint32_t GetEvents() const;
};