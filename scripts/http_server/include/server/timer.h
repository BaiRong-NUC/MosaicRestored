// 基于时间轮的定时器实现
#pragma once
#include "utils/public.h"
#include "server/channel.h"

class EventLoop;

class TimerTask
{
   private:
    using Action = std::function<void()>;  // 定时器回调函数类型,无参无返回值
    uint64_t _id;                          // 定时器ID
    uint64_t _expireTime;                  // 定时器超时时间
    Action _timer_action;                  // 定时器回调函数
    Action _release;                       // 定时器释放函数,用于定时器到期后从时间轮中移除定时器对象
    bool _isActive;                        // 定时器是否被取消

   public:
    TimerTask(uint64_t id, uint64_t expireTime, Action action, Action release);
    uint64_t GetId() const;
    uint64_t GetExpireTime() const;
    void Cancel();          // 取消定时器,设置定时器状态为已取消
    bool IsActive() const;  // 获取定时器状态
    ~TimerTask();           // 定时器到期时执行回调函数
};

class Timer
{
   private:
    using Action = std::function<void()>;  // 定时器回调函数类型,无参无返回值
    using PtrTimerTask = std::shared_ptr<TimerTask>;
    struct WheelNode
    {
        uint64_t id;
        uint64_t version;
        uint64_t rounds;  // 还需要转过多少整圈才可到期
    };
    std::vector<std::vector<WheelNode>> _timeWheel;          // 时间轮,每个槽位存储定时器ID和版本
    size_t _tick;                                            // 到期指针,指向那块释放那块
    size_t _wheelSize;                                       // 时间轮大小(最大延时时间)
    std::unordered_map<uint64_t, PtrTimerTask> _taskMap;     // 定时器ID到定时器对象的映射(强引用)
    std::unordered_map<uint64_t, uint64_t> _taskVersionMap;  // 定时器ID到最新版本号的映射

    // int _timerfd; // 定时器文件描述符,用于触发定时器事件
    Channel _timerChannel;  // 监控定时器文件描述符的Channel对象,用于在定时器事件触发时执行回调函数

    // 将定时器操作全部放在EventLoop线程中执行,保证定时器成员变量的线程安全
    EventLoop *_eventLoop;  // 关联的EventLoop对象,用于在定时器事件触发时执行回调函数

    // 删除定时器
    void _Remove(uint64_t id, uint64_t version);

   public:
    Timer(EventLoop *eventLoop, size_t wheelSize = 60);
    size_t GetTickPtr() const;
    void Add(uint64_t id, uint64_t expireTime, Action action);
    void Refresh(uint64_t id, uint64_t newExpireTime = 0);
    void Tick();
    void Cancel(uint64_t id);

    bool Find(uint64_t id);  // 查找定时器,用于测试
};