#include "staticserver.h"

int main()
{  
    StaticServer::s_logger = spdlog::stdout_color_mt("main");
    StaticServer::create();
    if (!StaticServer::getInstance()->init())
    {
        return 1;
    }
    StaticServer::getInstance()->loop();
}

