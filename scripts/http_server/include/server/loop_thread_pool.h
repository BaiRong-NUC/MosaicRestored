#pragma once
#include "utils/public.h"
#include "server/loop_thread.h"

// LoopThreadPool: 针对LoopThred的线程池

/**
 * 1. 线程数量可配置,在主从Reactor模型中
 *      主线程池负责监听和接受连接,从线程池负责处理连接的IO事件和业务逻辑.
 * 2. 在轻量场景中,LoopThreadPool可能没有线程,此时主线程既负责监听和接受连接,也负责处理连接的IO事件和业务逻辑.
 * 3. 管理ThreadPool对象
 * 4. 提供线程分配接口,根据将连接分配给不同的线程处理对应的接口(轮转分配)
 */

class LoopThreadPool
{
   private:
    int _sub_thread_num;                                    // 从属线程数量
    EventLoop *_base_loop;                                  // 主线程的EventLoop
    std::vector<std::unique_ptr<LoopThread>> _sub_threads;  // 从属线程列表
    // 从属线程的EventLoop列表,方便分配连接时获取EventLoop对象,因为从LoopThread获取时有锁消耗
    std::vector<EventLoop *> _sub_event_loops;
    int _next_index;          // 轮转分配的下一个线程数组索引
    std::mutex _index_mutex;  // 保护轮转索引,支持并发分配
   public:
    LoopThreadPool(EventLoop *base_loop, int thread_num = 0);

    ~LoopThreadPool();

    EventLoop *GetSubEventLoop();  // 获取下一个线程的EventLoop对象,用于管理连接

    EventLoop *GetBaseLoop();  // 获取主线程的EventLoop对象,用于监听和接受连接
};