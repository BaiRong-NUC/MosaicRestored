#include "server/event_loop.h"
#include "server/timer.h"

// 执行任务队列中的所有任务
void EventLoop::_RunAllTasks()
{
    std::vector<Action> tasks;
    {
        // 获取锁,并且自动管理
        std::unique_lock<std::mutex> lock(this->_queue_mutex);
        tasks.swap(this->_tasks_queue);  // 交换任务队列,避免长时间持有锁
    }

    for (const Action &task : tasks)
    {
        task();  // 执行任务
    }
}

EventLoop::~EventLoop() { delete this->_timer; }

EventLoop::EventLoop()
    : _poller(),
      _efd_channel(this, std::move(Socket(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)))),
      _quit(false),
      _thread_id(std::this_thread::get_id())
{
    if (this->_efd_channel.GetSocket().GetSocketFd() < 0)
    {
        LOG(ERROR, "Failed to create eventfd");
        exit(EXIT_FAILURE);
    }

    this->_timer = new Timer(this);  // 创建定时器对象,用于管理定时器任务

    // 当事件fd可读时说明有任务需要处理
    this->_efd_channel.readAction = [this]()
    {
        uint64_t one;
        // ssize_t size = this->_efd_channel.GetSocket().Recv(&one, sizeof(one)); // 读取事件fd,清除可读事件
        ssize_t size = read(this->_efd_channel.GetSocket().GetSocketFd(), &one,
                            sizeof(one));  // 直接使用read读取事件fd,清除可读事件
        if (size < 0)
        {
            LOG(ERROR, "Failed to read from eventfd");
            exit(EXIT_FAILURE);
        }
        // 读取成功,计数清零
    };

    this->_efd_channel.EnableRead();  // 启动事件fd的可读事件监控,当可读时说明有任务需要处理
}

void EventLoop::Start()
{
    _quit.store(false);
    while (!_quit.load())
    {
        // 事件监控
        std::vector<Channel *> activeChannels = this->_poller.Poll(-1);
        for (Channel *channel : activeChannels)
        {
            channel->HandleEvent();  // 就绪事件处理
        }
        // 执行任务
        this->_RunAllTasks();
    }
}

void EventLoop::Stop()
{
    _quit.store(true);

    // 唤醒可能阻塞在epoll上的事件循环线程,使其尽快退出
    uint64_t one = 1;
    ssize_t size = write(this->_efd_channel.GetSocket().GetSocketFd(), &one, sizeof(one));
    if (size < 0 && errno != EAGAIN)
    {
        LOG(ERROR, "Failed to write to eventfd while stopping loop");
    }
}

// 判断当前线程是否是事件循环所在的线程
bool EventLoop::InLoop() { return this->_thread_id == std::this_thread::get_id(); }

// 将不在EventLoop线程的任务压入队列中
void EventLoop::AddTask(const Action &task)
{
    {
        std::unique_lock<std::mutex> lock(this->_queue_mutex);
        this->_tasks_queue.push_back(task);  // 将任务添加到队列中
    }
    // 唤醒可能因为没有事件就绪而导致的epoll阻塞,否则Start->RunAllTasks可能无法及时执行
    uint64_t one = 1;
    // ssize_t size = this->_efd_channel.GetSocket().Send(&one, sizeof(one)); // 写入事件fd,触发可读事件
    ssize_t size =
        write(this->_efd_channel.GetSocket().GetSocketFd(), &one, sizeof(one));  // 直接使用write写入事件fd,触发可读事件
    if (size < 0)
    {
        LOG(ERROR, "Failed to write to eventfd");
        exit(EXIT_FAILURE);
    }
}

// 运行任务
void EventLoop::RunTask(const Action &task)
{
    if (this->InLoop() == true)
    {
        task();
        return;
    }
    // 当前任务不在EventLoop线程上
    this->AddTask(task);  // 将任务添加到队列中
}

// 在Poller中移除channel事件监控
void EventLoop::AddChannel(Channel *channel) { this->_poller.AddChannel(channel); }

// 从Poller中移除Channel的事件监控
void EventLoop::RemoveChannel(Channel *channel) { this->_poller.RemoveChannel(channel); }

// 添加定时器任务
void EventLoop::AddTimerTask(uint64_t id, uint64_t expireTime, Action action)
{
    this->_timer->Add(id, expireTime, action);
}

// 刷新定时器任务的到期时间
void EventLoop::RefreshTimerTask(uint64_t id, uint64_t newExpireTime) { this->_timer->Refresh(id, newExpireTime); }

// 取消定时器任务
void EventLoop::CancelTimerTask(uint64_t id) { this->_timer->Cancel(id); }

// 查找定时器任务,用于测试
bool EventLoop::FindTimerTask(uint64_t id) { return this->_timer->Find(id); }
