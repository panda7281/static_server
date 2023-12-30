#pragma once

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "threadpool.h"
#include "httpheaderparser.h"
#include "httpclienttask.h"
#include "hashedwheeltimer.h"
#include "filecachepool.h"
#include "utils.h"

class StaticServer
{
public:
    ~StaticServer();

    bool init();
    void loop();

    static void create();
    static StaticServer* getInstance();

    static std::shared_ptr<spdlog::logger> s_logger;

private:
    std::vector<HTTPClientTask> m_clients;
    std::vector<Timer> m_timers;
    std::vector<epoll_event> m_epevents;
    sockaddr_in m_addr;
    
    int m_epfd;
    int m_listenfd;
    int m_timerinterval;
    int m_connection_timeout;
    bool m_stop_server = false, m_timeout_flag = false;

    std::shared_ptr<ThreadPool> m_tp;
    std::shared_ptr<FileCachePool> m_fp;
    std::shared_ptr<HashedWheelTimer> m_timer;

    StaticServer();
    static StaticServer* s_instance;
    static int s_fd_sigpipe[2];
    static void sighandler(int sig);
    void timeoutcb(HTTPClientTask *client);
};
