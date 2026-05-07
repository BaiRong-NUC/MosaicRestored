#include "server/task_thread_pool.h"

#include "utils/log.h"

TaskThreadPool::TaskThreadPool(size_t thread_num) : _next_worker(0)
{
    _workers.reserve(thread_num);
    for (size_t i = 0; i < thread_num; ++i)
    {
        std::unique_ptr<Worker> worker(new Worker());
        Worker *worker_ptr = worker.get();
        worker_ptr->thread = std::thread([this, worker_ptr]()
                                         { this->_RunWorker(worker_ptr); });
        _workers.push_back(std::move(worker));
    }
}

TaskThreadPool::~TaskThreadPool()
{
    for (const auto &worker : _workers)
    {
        {
            std::unique_lock<std::mutex> lock(worker->mutex);
            worker->stopping = true;
        }
        worker->condition.notify_one();
    }

    for (const auto &worker : _workers)
    {
        if (worker->thread.joinable())
        {
            worker->thread.join();
        }
    }
}

size_t TaskThreadPool::Size() const { return _workers.size(); }

void TaskThreadPool::Submit(Task task)
{
    if (_workers.empty())
    {
        task();
        return;
    }

    size_t index = _next_worker.fetch_add(1, std::memory_order_relaxed) % _workers.size();
    this->_SubmitToWorker(index, std::move(task));
}

void TaskThreadPool::Submit(uint64_t key, Task task)
{
    if (_workers.empty())
    {
        task();
        return;
    }

    this->_SubmitToWorker(static_cast<size_t>(key % _workers.size()), std::move(task));
}

void TaskThreadPool::_SubmitToWorker(size_t index, Task task)
{
    Worker *worker = _workers[index].get();
    {
        std::unique_lock<std::mutex> lock(worker->mutex);
        worker->tasks.push_back(std::move(task));
    }
    worker->condition.notify_one();
}

void TaskThreadPool::_RunWorker(Worker *worker)
{
    while (true)
    {
        Task task;
        {
            std::unique_lock<std::mutex> lock(worker->mutex);
            worker->condition.wait(lock, [worker]()
                                   { return worker->stopping || !worker->tasks.empty(); });
            if (worker->stopping && worker->tasks.empty())
            {
                return;
            }
            task = std::move(worker->tasks.front());
            worker->tasks.pop_front();
        }

        try
        {
            task();
        }
        catch (const std::exception &error)
        {
            LOG(ERROR, "Unhandled task exception: " << error.what());
        }
        catch (...)
        {
            LOG(ERROR, "Unhandled unknown task exception");
        }
    }
}