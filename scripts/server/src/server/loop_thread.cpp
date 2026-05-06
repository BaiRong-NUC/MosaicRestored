#include "server/loop_thread.h"

LoopThread::LoopThread()
    : _event_loop(nullptr),
      _exited(false),
      _thread(
          [this]()
          {
              // 上线
              EventLoop loop;  // 在线程中实例化EventLoop对象
              {
                  std::unique_lock<std::mutex> lock(this->_mutex);
                  this->_event_loop = &loop;     // 获取线程内的EventLoop对象
                  this->_cond_var.notify_all();  // 通知GetEventLoop()可以获取_event_loop了
              }
              loop.Start();  // 启动事件循环

              // loop.Start()内部是循环判断的,到这里代表event_loop quit标记为true了,事件循环退出了,线程函数即将退出了

              // 下线
              {
                  std::unique_lock<std::mutex> lock(this->_mutex);
                  this->_event_loop = nullptr;
                  this->_exited = true;
                  this->_cond_var.notify_all();
              }
          })
{
}

LoopThread::~LoopThread()
{
    if (_thread.joinable())
    {
        EventLoop* loop = this->GetEventLoop();
        if (loop != nullptr)
        {
            loop->Stop();
        }
        _thread.join();
    }
}

EventLoop* LoopThread::GetEventLoop()
{
    EventLoop* loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(this->_mutex);
        while (this->_event_loop == nullptr && !this->_exited)
        {
            this->_cond_var.wait(lock);  // 等待_event_loop实例化完成
        }
        loop = this->_event_loop;  // 获取线程内的EventLoop对象
    }
    return loop;
}