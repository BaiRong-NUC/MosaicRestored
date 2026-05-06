#include "server/loop_thread_pool.h"

LoopThreadPool::LoopThreadPool(EventLoop *base_loop, int thread_num)
    : _sub_thread_num(thread_num < 0 ? 0 : thread_num), _base_loop(base_loop), _next_index(0)
{
    assert(_base_loop != nullptr);
    assert(thread_num >= 0);

    // 创建从属线程对象
    for (int i = 0; i < _sub_thread_num; ++i)
    {
        std::unique_ptr<LoopThread> sub_thread(new LoopThread());
        _sub_threads.push_back(std::move(sub_thread));
        // 没有初始化完EventLoop对象时会阻塞
        _sub_event_loops.push_back(_sub_threads.back()->GetEventLoop());
    }
}

LoopThreadPool::~LoopThreadPool() {}

EventLoop *LoopThreadPool::GetSubEventLoop()
{
    if (_sub_event_loops.empty())
    {
        return _base_loop;  // 没有从属线程时返回主线程的EventLoop对象
    }

    std::unique_lock<std::mutex> lock(_index_mutex);
    EventLoop *loop = _sub_event_loops[_next_index];            // 获取下一个线程的EventLoop对象
    _next_index = (_next_index + 1) % _sub_event_loops.size();  // 轮转分配下一个线程数组索引
    return loop;
}

EventLoop *LoopThreadPool::GetBaseLoop()
{
    return _base_loop;  // 获取主线程的EventLoop对象
}