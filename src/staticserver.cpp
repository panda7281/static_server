#include "staticserver.h"

#include <nlohmann/json.hpp>
#include <fstream>

StaticServer *StaticServer::s_instance = nullptr;
int StaticServer::s_fd_sigpipe[2];
std::shared_ptr<spdlog::logger> StaticServer::s_logger;

StaticServer::StaticServer()
{
    m_clients.resize(MAX_FD_SIZE);
    m_timers.resize(MAX_FD_SIZE);
    m_epevents.resize(MAX_FD_SIZE);
}

StaticServer::~StaticServer()
{
}

void StaticServer::create()
{
    if (s_instance)
        return;
    s_instance = new StaticServer();
}

StaticServer *StaticServer::getInstance()
{
    return s_instance;
}

bool StaticServer::init()
{
    // 读取配置文件
    std::filesystem::path configPath = std::filesystem::current_path() / "config/config.json";
    if (!std::filesystem::is_regular_file(configPath))
    {
        s_logger->critical("[init] config file don't exist");
        return false;
    }
    nlohmann::json configJson;
    std::ifstream configFStream(configPath);
    configFStream >> configJson;
    if (configJson.empty())
    {
        s_logger->critical("[init] fail to read {}", configPath.string());
        return false;
    }
    s_logger->info("-------------------------------");
    s_logger->info("[init] reading configurations");
    // 设置日志等级
    std::string loglevelstr = configJson["loglevel"].is_string() ? configJson["loglevel"].get<std::string>() : "";
    spdlog::set_level(Utils::str2loglvl(loglevelstr));
    s_logger->info("[init] log level is set to {}", loglevelstr);
    // 设置监听地址和端口
    std::string host = configJson["host"].is_string() ? configJson["host"].get<std::string>() : "0.0.0.0";
    uint16_t port = configJson["port"].is_number_unsigned() ? configJson["port"].get<uint16_t>() : 12345;
    s_logger->info("[init] host and port is set to {}:{}", host, port);
    // backlog size
    int backlog = configJson["backlog"].is_number_unsigned() ? configJson["backlog"].get<int>() : 5;
    s_logger->info("[init] backlog size is set to {}", backlog);
    // 连接超时
    m_connection_timeout = configJson["timeout"].is_number_unsigned() ? configJson["timeout"].get<int>() : 10;
    s_logger->info("[init] connection timeout is set to {}s", m_connection_timeout);
    // 设置服务器的根目录
    std::string root = configJson["root"].is_string() ? configJson["root"].get<std::string>() : "www";
    s_logger->info("[init] doc root is set to {}", root);
    // 设置线程池的参数
    int tpworker = std::thread::hardware_concurrency(), tpmaxtasks = 10000;
    if (configJson["threadpool"].is_object())
    {
        const auto &tpconfigjson = configJson["threadpool"];
        tpworker = tpconfigjson["workers"].is_number_unsigned() ? tpconfigjson["workers"].get<int>() : std::thread::hardware_concurrency();
        tpmaxtasks = tpconfigjson["maxtask"].is_number_unsigned() ? tpconfigjson["maxtask"].get<int>() : 10000;
    }
    s_logger->info("[init] thread pool: workers={}, maxtask={}", tpworker, tpmaxtasks);
    // 设置缓存池的参数
    int cpmaxsize = 1073741824, cpmaxitems = 10000;
    if (configJson["cachepool"].is_object())
    {
        const auto &cpconfigjson = configJson["cachepool"];
        cpmaxsize = cpconfigjson["maxsize"].is_number_unsigned() ? cpconfigjson["maxsize"].get<int>() : 1073741824;
        cpmaxitems = cpconfigjson["maxitem"].is_number_unsigned() ? cpconfigjson["maxitem"].get<int>() : 10000;
    }
    s_logger->info("[init] file cache pool: maxsize={} bytes, maxitem={}", cpmaxsize, cpmaxitems);
    // 设置时钟的参数
    int timergranularity = 10;
    m_timerinterval = 1;
    if (configJson["timer"].is_object())
    {
        const auto &timerconfigjson = configJson["timer"];
        timergranularity = timerconfigjson["granularity"].is_number_unsigned() ? timerconfigjson["granularity"].get<int>() : 10;
        m_timerinterval = timerconfigjson["interval"].is_number_unsigned() ? timerconfigjson["interval"].get<int>() : 1;
    }
    s_logger->info("[init] timer: granularity={}, interval={}s", timergranularity, m_timerinterval);
    s_logger->info("-------------------------------");
    // 开始监听
    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
    struct linger tmp = {1, 0};
    setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    if (m_listenfd < 0)
    {
        s_logger->critical("[init] fail to create listen socket");
        return false;
    }
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &m_addr.sin_addr);
    int ret = bind(m_listenfd, reinterpret_cast<sockaddr *>(&m_addr), sizeof(m_addr));
    if (ret == -1)
    {
        s_logger->critical("[init] fail to bind listen socket to {}", Utils::addr2str(m_addr));
        return false;
    }
    ret = listen(m_listenfd, backlog);
    if (ret == -1)
    {
        s_logger->critical("[init] fail to listen on {}", Utils::addr2str(m_addr));
        return false;
    }
    // 创建线程池
    m_tp = std::make_shared<ThreadPool>();
    m_tp->start(tpworker, tpmaxtasks);
    // 创建文件缓存池
    m_fp = std::make_shared<FileCachePool>(cpmaxsize, cpmaxitems);
    // 创建计时器
    m_timer = std::make_shared<HashedWheelTimer>(timergranularity, std::chrono::seconds(m_timerinterval));

    // 建立处理信号用的管道
    socketpair(PF_UNIX, SOCK_STREAM, 0, s_fd_sigpipe);
    // 将管道的写入设为非阻塞
    Utils::setfdnonblocking(s_fd_sigpipe[1]);
    // 创建epoll描述符
    m_epfd = epoll_create(MAX_EVENT_SIZE);
    // 监听signal管道的读取端
    Utils::addepfd(m_epfd, s_fd_sigpipe[0], EPOLLIN | EPOLLET);
    // 监听listenfd
    Utils::addepfd(m_epfd, m_listenfd, EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLHUP);
    Utils::setfdnonblocking(m_listenfd);

    // 添加信号处理函数
    Utils::setsighandler(SIGINT, &sighandler);
    Utils::setsighandler(SIGTERM, &sighandler);
    Utils::setsighandler(SIGPIPE, &sighandler);
    Utils::setsighandler(SIGALRM, &sighandler);

    // 给HTTP处理类设置参数
    HTTPClientTask::s_userCnt.fetch_and(0);
    HTTPClientTask::s_docRoot = root;
    HTTPClientTask::s_epfd = m_epfd;
    HTTPClientTask::s_pool = m_fp;
    HTTPClientTask::s_client_lock = std::vector<std::mutex>(MAX_FD_SIZE);
    HTTPClientTask::s_logger = s_logger;

    return true;
}

void StaticServer::loop()
{
    s_logger->info("[server] listening on {}", Utils::addr2str(m_addr));

    alarm(m_timerinterval);
    m_stop_server = false;
    m_timeout_flag = false;
    while (!m_stop_server)
    {
        int event_num = epoll_wait(m_epfd, &m_epevents[0], m_epevents.size(), -1);
        for (int nr_ev = 0; nr_ev < event_num; nr_ev++)
        {
            const auto &curr_event = m_epevents[nr_ev];
            int curr_fd = curr_event.data.fd;

            if (curr_fd == m_listenfd)
            {
                // 有新的传入连接
                sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(sockaddr_in);
                int clientfd = -1;
                // 循环接受连接，直到accept返回-1
                for (;;)
                {
                    int clientfd = accept(m_listenfd, reinterpret_cast<sockaddr *>(&client_addr), &client_addr_len);
                    if (clientfd < 0)
                    {
                        s_logger->info("[server] accept return -1: {}", strerror(errno));
                        break;
                    }
                    // 添加一个任务用于初始化连接
                    m_tp->appendTask(std::bind(&HTTPClientTask::init, &m_clients[clientfd], clientfd, client_addr));
                    s_logger->info("[server] accept connection, socket: {}, addr: {}", clientfd, Utils::addr2str(client_addr));
                    // 连接的超时计时器
                    m_timers[clientfd].time_point = std::chrono::high_resolution_clock::now() + std::chrono::seconds(m_connection_timeout);
                    m_timers[clientfd].callback = std::bind(&StaticServer::timeoutcb, this, &m_clients[clientfd]);
                    m_timer->addTimer(&m_timers[clientfd]);
                }
            }
            else if (curr_fd == s_fd_sigpipe[0])
            {
                // 有新的信号
                std::array<char, 512> sigbuff;
                int sigcnt = recv(s_fd_sigpipe[0], &sigbuff[0], sigbuff.size(), 0);
                for (int nr_sig = 0; nr_sig < sigcnt; nr_sig++)
                {
                    switch (sigbuff[nr_sig])
                    {
                    case SIGINT:
                        m_stop_server = true;
                        s_logger->info("[server] SIGINT received, exiting");
                        break;
                    case SIGTERM:
                        s_logger->info("[server] SIGTERM received, exiting");
                        m_stop_server = true;
                        break;
                    case SIGALRM:
                        m_timeout_flag = true;
                        s_logger->trace("[server] SIGALRM received");
                        break;
                    case SIGPIPE:
                        s_logger->info("[server] SIGPIPE received, do nothing");
                        break;
                    default:
                        s_logger->info("[server] unregistered signal {} received, do nothing", sigbuff[nr_sig]);
                        break;
                    }
                }
            }
            else if (curr_event.events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                // 连接出错，提交一个任务用于关闭连接
                m_tp->appendTask(std::bind(&HTTPClientTask::process, &m_clients[curr_fd], HTTPClientTask::TaskContext::CLOSE));
                s_logger->warn("[server] socket: {} received EPOLLRDHUP/EPOLLHUP/EPOLLERR, closing", curr_fd);
                // 删掉socket上的定时器
                m_timer->delTimer(&m_timers[curr_fd]);
            }
            else
            {
                if (curr_event.events & EPOLLIN)
                {
                    // 有待读取的数据
                    m_tp->appendTask(std::bind(&HTTPClientTask::process, &m_clients[curr_fd], HTTPClientTask::TaskContext::IN));
                    s_logger->trace("[server] socket: {}, EPOLLIN", curr_fd);
                }
                else if (curr_event.events & EPOLLOUT)
                {
                    // socket上的可写任务
                    m_tp->appendTask(std::bind(&HTTPClientTask::process, &m_clients[curr_fd], HTTPClientTask::TaskContext::OUT));
                    s_logger->trace("[server] socket: {}, EPOLLOUT", curr_fd);
                }
                // 更新socket上的定时器
                m_timers[curr_fd].time_point = std::chrono::high_resolution_clock::now() + std::chrono::seconds(m_connection_timeout);
                m_timers[curr_fd].callback = std::bind(&StaticServer::timeoutcb, this, &m_clients[curr_fd]);
                m_timer->delTimer(&m_timers[curr_fd]);
                m_timer->addTimer(&m_timers[curr_fd]);
            }
        }
        if (m_timeout_flag)
        {
            m_timer->tick();
            alarm(m_timerinterval);
            m_timeout_flag = false;
        }
    }

    close(m_listenfd);
    close(s_fd_sigpipe[1]);
    close(s_fd_sigpipe[0]);
    close(m_epfd);
}

void StaticServer::sighandler(int sig)
{
    // 保存errno，因为send可能会设置errno
    int errno_org = errno;
    char sig_chr = sig;
    send(s_fd_sigpipe[1], &sig_chr, 1, 0);
    // 还原errno
    errno = errno_org;
}

void StaticServer::timeoutcb(HTTPClientTask *client)
{
    m_tp->appendTask(std::bind(&HTTPClientTask::process, client, HTTPClientTask::TaskContext::CLOSE));
    s_logger->info("timeout callback triggered");
}
