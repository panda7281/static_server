#include "hashedwheeltimer.h"

HashedWheelTimer::HashedWheelTimer(int bucket_cnt, std::chrono::nanoseconds interval)
{
    m_wheel.resize(bucket_cnt);
    m_interval = interval;
    m_div = 0;
}

HashedWheelTimer::~HashedWheelTimer()
{
    for (auto head : m_wheel)
    {
        while (head)
        {
            auto next = head->next;
            delete head;
            head = next;
        }
    }
}

void HashedWheelTimer::addTimer(Timer *t)
{
    // 如果t已经在计时器中了，则先将其删除
    if (m_posMap.contains(t))
    {
        delTimer(t);
    }
    auto tp_now = std::chrono::high_resolution_clock::now();
    int total_divs = (t->time_point - tp_now) / m_interval;
    int rot = total_divs / m_wheel.size();
    int div = (m_div + total_divs) % m_wheel.size();
    
    TimerContainerNode* new_node = new TimerContainerNode();
    new_node->t = t;
    new_node->rot = rot;
    new_node->div = div;
    
    // 将新节点插入到首节点之前
    TimerContainerNode* old_head = m_wheel[div];
    new_node->next = old_head;
    if (old_head)
        old_head->prev = new_node;
    m_wheel[div] = new_node;

    // 添加到映射
    m_posMap[t] = new_node;
}

void HashedWheelTimer::delTimer(Timer *t)
{
    if (!m_posMap.contains(t))
        return;
    auto ptr = m_posMap[t];

    if (ptr->next)
        ptr->next->prev = ptr->prev;
    
    if (ptr->prev)
    {
        ptr->prev->next = ptr->next;
    }
    else
    {
        // 如果ptr是链表头，则需要重设链表地址
        m_wheel[ptr->div] = ptr->next;
    }

    delete ptr;
    m_posMap.erase(t);
}

void HashedWheelTimer::tick()
{
    TimerContainerNode* curr_node = m_wheel[m_div];

    while (curr_node)
    {
        TimerContainerNode* next_node = curr_node->next;
        if (curr_node->rot > 0)
        {
            curr_node->rot--;
        } 
        else 
        {
            curr_node->t->callback();
            delTimer(curr_node->t);
        }
        curr_node = next_node;
    }
    
    m_div = (m_div + 1) % m_wheel.size();
}
