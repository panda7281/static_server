#pragma once

#include <chrono>
#include <functional>
#include <vector>
#include <list>
#include <unordered_map>

#include "httpclienttask.h"

struct Timer
{
    std::chrono::time_point<std::chrono::high_resolution_clock> time_point;
    std::function<void()> callback;
};

class HashedWheelTimer
{
public:
    // 给定一圈的分割数和时间间隔
    HashedWheelTimer(int bucket_cnt, std::chrono::nanoseconds interval);
    ~HashedWheelTimer();
    void addTimer(Timer* t);
    void delTimer(Timer* t);
    void tick();
private:
    // 时间轮中的实际项，额外存储了圈数和前后的指针
    struct TimerContainerNode
    {
        Timer* t;
        int rot;
        int div;
        TimerContainerNode* prev = nullptr;
        TimerContainerNode* next = nullptr;
    };

    // 间隔
    std::chrono::nanoseconds m_interval;
    // 时间轮
    std::vector<TimerContainerNode*> m_wheel;
    // 当前所在的位置
    int m_div;
    // 维护Timer指针到链表节点的映射，确保O(1)的删除和修改复杂度
    std::unordered_map<Timer*, TimerContainerNode*> m_posMap;
};

