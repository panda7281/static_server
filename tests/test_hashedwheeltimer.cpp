#include <catch2/catch_all.hpp>
#include "hashedwheeltimer.h"
#include <chrono>
#include <vector>
#include <unordered_set>

TEST_CASE("Hash Wheel Timer", "[basic]")
{
    HashedWheelTimer timer(10, std::chrono::seconds(1));
    bool flags[20];
    memset(flags, 0, sizeof(bool)*20);
    
    std::vector<Timer> timers(20);
    
    std::function<void(bool*)> cb = [](bool* flag)
    {
        *flag = true;
    };

    for (int i = 0; i < timers.size(); i++)
    {
        flags[i] = false;
        timers[i].callback = std::bind(cb,  &flags[i]);
        timers[i].time_point = std::chrono::high_resolution_clock::now() + std::chrono::seconds(i);
        timer.addTimer(&timers[i]);
    }

    for (int i = 0; i < timers.size() + 1; i++)
    {
        timer.tick();
    }
    
    for (int i = 0; i < timers.size(); i++)
    {
        REQUIRE(flags[i] == true);
    }

}

TEST_CASE("Hash Wheel Timer", "[remove timer]")
{
    HashedWheelTimer timer(10, std::chrono::seconds(1));
    bool flags[20];
    memset(flags, 0, sizeof(bool)*20);
    
    std::vector<Timer> timers(20);
    std::unordered_set<int> timer_del = {2, 7, 11};
    
    std::function<void(bool*)> cb = [](bool* flag)
    {
        *flag = true;
    };

    for (int i = 0; i < timers.size(); i++)
    {
        flags[i] = false;
        timers[i].callback = std::bind(cb,  &flags[i]);
        timers[i].time_point = std::chrono::high_resolution_clock::now() + std::chrono::seconds(i);
        timer.addTimer(&timers[i]);
    }

    // remove 3 timer
    for (auto& del_index : timer_del)
    {
        timer.delTimer(&timers[del_index]);
    }


    for (int i = 0; i < timers.size() + 1; i++)
    {
        timer.tick();
    }
    
    for (int i = 0; i < timers.size(); i++)
    {
        if (timer_del.count(i))
        {
            REQUIRE(flags[i] == false);
        }
        else
        {
            REQUIRE(flags[i] == true);
        }
        
    }
}

TEST_CASE("Hash Wheel Timer", "[duplicate timer]")
{
    HashedWheelTimer timer(10, std::chrono::seconds(1));
    bool flags[20];
    memset(flags, 0, sizeof(bool)*20);
    
    std::vector<Timer> timers(20);
    
    std::function<void(bool*)> cb = [](bool* flag)
    {
        *flag = true;
    };

    for (int i = 0; i < timers.size(); i++)
    {
        timers[i].callback = std::bind(cb,  &flags[i]);
        timers[i].time_point = std::chrono::high_resolution_clock::now() + std::chrono::seconds(1000);
        timer.addTimer(&timers[i]);
    }

    // change timepoint
    for (int i = 0; i < timers.size(); i++)
    {
        timers[i].time_point = std::chrono::high_resolution_clock::now() + std::chrono::seconds(1);
        timer.addTimer(&timers[i]);
    }

    for (int i = 0; i < timers.size() + 1; i++)
    {
        timer.tick();
    }
    
    for (int i = 0; i < timers.size(); i++)
    {
        REQUIRE(flags[i] == true);
    }
}