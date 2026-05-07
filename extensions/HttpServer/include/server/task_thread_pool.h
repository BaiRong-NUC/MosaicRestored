#pragma once

#include "utils/public.h"

#include <deque>

class TaskThreadPool
{
private:
    using Task = std::function<void()>;

    struct Worker
    {
        std::thread thread;
        std::mutex mutex;
        std::condition_variable condition;
        std::deque<Task> tasks;
        bool stopping = false;
    };

    std::vector<std::unique_ptr<Worker>> _workers;
    std::atomic<size_t> _next_worker;

    void _RunWorker(Worker *worker);
    void _SubmitToWorker(size_t index, Task task);

public:
    explicit TaskThreadPool(size_t thread_num = 0);
    ~TaskThreadPool();

    size_t Size() const;
    void Submit(Task task);
    void Submit(uint64_t key, Task task);
};
