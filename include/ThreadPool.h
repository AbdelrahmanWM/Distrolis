#include<functional>
#include<condition_variable>
#include<mutex>
#include<queue>
#include<thread>

#ifndef THREADPOOL_H
#define THREADPOOL_H
class ThreadPool
{
public:
ThreadPool(size_t threads);
~ThreadPool();
void enqueue(std::function<void()> task);
void WorkerThread();
private:
std::vector<std::thread>workers;
std::queue<std::function<void()>> tasks;
std::mutex queueMutex;
std::condition_variable condition;
bool stop;
};

#endif