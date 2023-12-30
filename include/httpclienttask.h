#pragma once

#include <spdlog/spdlog.h>

#include "filecachepool.h"
#include "threadpool.h"
#include "httpheaderparser.h"
#include "utils.h"
#include "constants.h"
#include <atomic>
#include <filesystem>

#include <netinet/in.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/uio.h>

class HTTPClientTask
{
public:
    enum class TaskContext
    {
        IN,
        OUT,
        CLOSE
    };

    HTTPClientTask(/* args */);
    ~HTTPClientTask();
    // 在任务队列中调用的处理函数
    void process(TaskContext context);
    // 初始化一个连接
    void init(int sockfd, sockaddr_in& addr);
    // 断开与客户端之间的连接
    void close();

    static int s_epfd;
    static std::atomic<int> s_userCnt;
    static std::filesystem::path s_docRoot;
    static std::shared_ptr<FileCachePool> s_pool;
    static std::vector<std::mutex> s_client_lock;
    static std::shared_ptr<spdlog::logger> s_logger;

private:
    HTTPHeaderParser m_parser;
    int m_sockfd = -1;
    sockaddr_in m_addr;
    std::array<char, READ_BUFFER_SIZE> m_readBuf;
    int m_readIdx;
    int m_remainBytes;

    std::string* m_respond_header = nullptr;
    bool m_keep_connection = false;
    
    iovec m_iv[2];

    std::shared_ptr<FileCacheItem> m_fcont;
    
    void processRead();
    void processWrite();
    bool read();
};
