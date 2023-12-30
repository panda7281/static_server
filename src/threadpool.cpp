#include "threadpool.h"

ThreadPool::ThreadPool()
    : m_queueStatus {0}, m_stopFlag(true)
{
}

ThreadPool::~ThreadPool()
{
    stop();
}

void ThreadPool::start(int nr_threads, int nr_max_task)
{
    if (!m_stopFlag)
    {
        return;
    }
    m_stopFlag = false;
    m_nrMaxTask = nr_max_task;
    for (int i = 0; i < nr_threads; i++)
    {
        m_threads.emplace_back(std::bind(&ThreadPool::loop, this));
    }
}

void ThreadPool::wait()
{
    for (;;)
    {
        std::scoped_lock locker(m_taskQueueMutex);
        if (m_taskQueue.empty())
        {
            break;
        }
    }
}

bool ThreadPool::appendTask(std::function<void()> task)
{
    // 获取任务队列的锁 RAII
    std::scoped_lock locker(m_taskQueueMutex);
    if (m_taskQueue.size() >= m_nrMaxTask)
    {
        return false;
    }
    m_taskQueue.push(std::move(task));
    m_queueStatus.release();
    return true;
}

void ThreadPool::stop()
{
    // 设置停止标志
    if (m_stopFlag)
    {
        return;
    }
    m_stopFlag = true;
    // 信号量+N，确保唤醒所有线程并终止
    m_queueStatus.release(m_threads.size());
    // 回收所有线程
    for (auto& t: m_threads)
    {
        t.join();
    }
    m_threads.clear();
    // 清空任务队列
    while (!m_taskQueue.empty())
    {
        m_taskQueue.pop();
    }
    // 重置信号量
    m_queueStatus.~counting_semaphore();
    new (&m_queueStatus) std::counting_semaphore<INT_MAX> {0};
}

void ThreadPool::loop()
{
    while (!m_stopFlag)
    {
        // 信号量-1
        m_queueStatus.acquire();
        std::function<void()> curr_task;
        {
            std::scoped_lock locker(m_taskQueueMutex);
            if (m_taskQueue.empty())
            {
                continue;
            }
            curr_task = m_taskQueue.front();
            m_taskQueue.pop();
        }
        // 处理任务
        curr_task();
    }
}
