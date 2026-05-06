#pragma once
#include "utils/public.h"
#include "server/event_loop.h"
// 将EventLoop模块与线程绑定: 创建线程->在线程中实例化EventLoop对象

class LoopThread
{
   private:
    EventLoop* _event_loop;             // 线程内的EventLoop对象,在线程中实例化
    std::thread _thread;                // 线程对象
    std::mutex _mutex;                  // 互斥锁,防止GetEventLoop()在_event_loop还未实例化时被调用
    std::condition_variable _cond_var;  // 条件变量,用于等待_event_loop实例化完成
    bool _exited;                       // 线程函数是否已退出
   public:
    LoopThread();

    ~LoopThread();

    EventLoop* GetEventLoop();  // 获取线程内的EventLoop对象
};