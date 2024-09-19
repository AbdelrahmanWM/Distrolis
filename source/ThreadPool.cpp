#include "ThreadPool.h"
#include <iostream>
ThreadPool::ThreadPool(size_t threads)
{
    stop = false;
    for (size_t i = 0; i < threads; i++)
    {
        std::function<void()> funcPtr = [this]
        { this->workerThread(); };
        std::thread t{funcPtr};
        workers.push_back(std::move(t));
    }
    // add threads with conditional lock waiting for tasks in the queue to exicute them
}

ThreadPool::~ThreadPool()
{
    // stops all threads and joins them to make sure they complete thier work
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true; // shared resource
        condition.notify_all();
    }
    for (std::thread &thread : workers)
    {
        thread.join();
    }
}

void ThreadPool::enqueue(std::function<void()> task)
{
    if (stop)
    {
        std::cerr << "Adding tasks to a disabled bool";
        return;
    }
    tasks.push(task);
    condition.notify_one();
}

void ThreadPool::workerThread()
{
    while (true)
    {
        std::function<void()> task{};
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this]
                           { return stop || !tasks.empty(); });
            if (stop && tasks.empty())
                return;
            task = std::move(tasks.front());
            tasks.pop();
        }
        task();
    }
}
