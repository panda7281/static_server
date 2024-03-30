#include <catch2/catch_all.hpp>
#include "test_utils.h"
#include "staticserver.h"
#include <filesystem>

namespace fs = std::filesystem;

TEST_CASE("Static Server", "[system test]")
{
    auto pid = fork();
    REQUIRE(pid >= 0);

    if (pid == 0)
    {
        // child process
        REQUIRE(chdir("../../") == 0);
        StaticServer::s_logger = spdlog::stdout_color_mt("main");
        StaticServer::create();
        REQUIRE(StaticServer::getInstance()->init() == true);
        StaticServer::getInstance()->loop();
    }
    else
    {
        // parent process
        sleep(1);
        for (const auto &entry : fs::directory_iterator("../../www"))
        {
            if (entry.is_regular_file())
            {
                std::string fname = entry.path().filename();
                INFO("Checking file " + fname);
                std::string wget_cmd = "wget -O " + fname + " http://127.0.0.1:8080/" + fname;
                REQUIRE(system(wget_cmd.c_str()) == 0);
                REQUIRE(compareFiles(entry.path(), fname) == true);
            }
        }

        REQUIRE(kill(pid, SIGTERM) == 0);
    }
}