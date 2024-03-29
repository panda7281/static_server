#include <catch2/catch_all.hpp>
#include "threadpool.h"

TEST_CASE("Threadpool", "[basic]")
{
    ThreadPool pool;
    pool.start(8, 1000);

    bool flags[20];
    memset(flags, 0, sizeof(bool)*20);

    std::function<void(bool*)> cb = [](bool* flag)
    {
        *flag = true;
    };

    for (int i = 0; i < 20; i++)
    {
        pool.appendTask(std::bind(cb, &flags[i]));
    }
    pool.wait();
    
    for (int i = 0; i < 20; i++)
    {
        REQUIRE(flags[i] == true);
    }
    
}