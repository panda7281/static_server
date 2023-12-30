#pragma once

#include <thread>
#include <semaphore>
#include <mutex>

#include <vector>
#include <queue>
#include <functional>

class ThreadPool
{
private:
    std::vector<std::thread> m_threads;
    std::queue<std::function<void()>> m_taskQueue;
    std::mutex m_taskQueueMutex;
    int m_nrMaxTask;
    std::counting_semaphore<INT_MAX> m_queueStatus;
    bool m_stopFlag;

public:
    ThreadPool();
    ~ThreadPool();
    // 启动线程池，nr_threads个工作线程，任务队列中最多包含nr_max_task和任务
    void start(int nr_threads, int nr_max_task);
    // 等待任务队列中的所有任务被处理完成
    void wait();
    // 停止线程池
    void stop();
    // 向任务队列中添加一个任务
    bool appendTask(std::function<void()> task);
    // 工作线程的主循环
    void loop();
};

