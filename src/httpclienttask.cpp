#include "httpclienttask.h"

std::filesystem::path HTTPClientTask::s_docRoot;
int HTTPClientTask::s_epfd;
std::atomic<int> HTTPClientTask::s_userCnt;
std::shared_ptr<FileCachePool> HTTPClientTask::s_pool;
std::vector<std::mutex> HTTPClientTask::s_client_lock;
std::shared_ptr<spdlog::logger> HTTPClientTask::s_logger;

HTTPClientTask::HTTPClientTask()
{
}

HTTPClientTask::~HTTPClientTask()
{
    close();
}

void HTTPClientTask::process(TaskContext context)
{
    std::scoped_lock locker(s_client_lock[m_sockfd]);
    switch (context)
    {
    case TaskContext::IN:
    {
        // 读取任务
        processRead();
    }
    break;
    case TaskContext::OUT:
    {
        // 写入任务
        processWrite();
    }
    break;
    case TaskContext::CLOSE:
    {
        close();
    }
    break;
    default:
        // ???
        break;
    }
}

void HTTPClientTask::init(int sockfd, sockaddr_in &addr)
{
    std::scoped_lock locker(s_client_lock[sockfd]);
    if (m_sockfd > 0)
        close();
    m_parser.setBuffer(m_readBuf.data());
    Utils::setfdnonblocking(sockfd);
    Utils::setreusefd(sockfd, true);
    Utils::addepfd(s_epfd, sockfd, EPOLLIN | EPOLLET | EPOLLONESHOT | EPOLLRDHUP);
    m_sockfd = sockfd;
    m_addr = addr;
    // 用户数量+1
    s_userCnt.fetch_add(1);
    s_logger->info("[client] socket {}: init, current client count: {}", m_sockfd, s_userCnt.load());
}

void HTTPClientTask::close()
{
    // 关闭socket
    if (m_sockfd < 0)
    {
        return;
    }
    
    Utils::delepfd(s_epfd, m_sockfd);
    ::close(m_sockfd);
    // 重置所有变量
    m_readBuf.fill('\0');
    m_readIdx = 0;
    m_remainBytes = 0;
    m_fcont.reset();
    m_iv[0].iov_len = 0;
    m_iv[0].iov_base = nullptr;
    m_iv[1].iov_len = 0;
    m_iv[1].iov_base = nullptr;
    // 用户数量-1
    s_userCnt.fetch_sub(1);
    s_logger->info("[client] socket {}: closed, current client count: {}", m_sockfd, s_userCnt.load());
    m_sockfd = -1;
    // 重置parser
    m_parser.reset();
}

void HTTPClientTask::processRead()
{
    // 读取任务：首先读取sockfd上的全部数据，然后格式化请求头，请求头格式化完毕后尝试将目标文件映射到内存中并注册写入事件
    auto read_ret = read();
    // 如果read返回false，则直接关闭连接
    if (!read_ret)
    {
        close();
        return;
    }
    auto parse_ret = m_parser.parseRequest(m_readIdx);
    switch (parse_ret)
    {
    case HTTPHeaderParser::Status::WORKING:
    {
        // 还有数据没有读入
        s_logger->trace("[client] socket {}: need more data", m_sockfd);
        Utils::modepfd(s_epfd, m_sockfd, EPOLLIN | EPOLLET | EPOLLONESHOT | EPOLLRDHUP);
    }
    break;
    case HTTPHeaderParser::Status::DONE:
    {
        // 数据已经读取完成
        s_logger->trace("[client] socket {}: header parse done", m_sockfd);
        // 重置接收缓存的位置
        m_readIdx = 0;
        auto req_header = m_parser.getRequestHeader();
        // 如果path为空，则加上index.html
        if (req_header->path.empty())
            req_header->path += "index.html";
        // 是否要保持连接
        m_keep_connection = req_header->opt.contains("Connection") &&  req_header->opt["Connection"].starts_with("keep-alive");
        s_logger->trace("[client] socket {}: keep connection: {}", m_sockfd, m_keep_connection);
        // 只支持GET方法和HTTP/1.1
        if (req_header->method != HTTPHeaderParser::Method::GET || req_header->version != "HTTP/1.1")
        {
            s_logger->trace("[client] socket {}: unsupport http method or version, respond BAD_REQUEST", m_sockfd);
            m_respond_header = m_parser.getRespondHeader("HTTP/1.1", HTTPHeaderParser::StatusCode::BAD_REQUEST, std::nullopt);
        }
        else
        {
            std::filesystem::path docPath = s_docRoot / req_header->path;
            s_logger->trace("[client] socket {}: requesting doc {}", m_sockfd, docPath.c_str());
            m_fcont = s_pool->getFile(docPath.string());
            
            if (!m_fcont)
            {
                // 请求的资源不存在
                s_logger->trace("[client] socket {}: doc not found, respond NOT_FOUND", m_sockfd);
                m_respond_header = m_parser.getRespondHeader("HTTP/1.1", HTTPHeaderParser::StatusCode::NOT_FOUND, std::nullopt);
            } else
            {
                HTTPHeaderParser::KVMap opt;
                if (m_keep_connection)
                    opt["Connection"] = "keep-alive";
                else
                    opt["Connection"] = "closed";
                // 根据文件的后缀名生成MIME格式
                if (mimeLookUpTable.contains(docPath.extension()))
                {
                    opt["Content-Type"] = mimeLookUpTable.at(docPath.extension());
                }
                else
                {
                    opt["Content-Type"] = "application/unknown";
                }
                opt["Content-Length"] = std::to_string(m_fcont->getStat()->st_size);
                m_respond_header = m_parser.getRespondHeader("HTTP/1.1", HTTPHeaderParser::StatusCode::OK, opt);
            }
        }
        m_parser.reset();
        // 准备写入
        m_iv[0].iov_base = (void*)m_respond_header->c_str();
        m_iv[0].iov_len = m_respond_header->size();
        if (m_fcont)
        {
            m_iv[1].iov_base = const_cast<void*>(m_fcont->getData());
            m_iv[1].iov_len = m_fcont->getStat()->st_size;
        } else 
        {
            m_iv[1].iov_base = nullptr;
            m_iv[1].iov_len = 0;
        }
        m_remainBytes = m_iv[0].iov_len + m_iv[1].iov_len;
        Utils::modepfd(s_epfd, m_sockfd, EPOLLOUT | EPOLLET | EPOLLONESHOT | EPOLLRDHUP);
    }
    break;
    case HTTPHeaderParser::Status::ERROR:
    {
        // 出现了错误，关闭连接
        close();
        s_logger->warn("[client] socket {}: header parse error, connection closed", m_sockfd);
    }
    break;
    default:
        // ??
        break;
    }
}

void HTTPClientTask::processWrite()
{
    // 执行writev
    // 循环写入
    while (m_remainBytes > 0) {
        ssize_t result = writev(m_sockfd, m_iv, 2);
        if (result > 0) {
            s_logger->trace("[client] socket {}: write {} bytes of data", m_sockfd, result);
            m_remainBytes -= result;
            // 更新iovector的指针
            for (int i = 0; i < 2; ++i) {
                if (result >= m_iv[i].iov_len) {
                    result -= m_iv[i].iov_len;
                    m_iv[i].iov_len = 0;
                } else {
                    m_iv[i].iov_base = reinterpret_cast<char*>(m_iv[i].iov_base) + result;
                    m_iv[i].iov_len -= result;
                    break;
                }
            }
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // 等待下一轮EPOLLOUT事件
            s_logger->trace("[client] socket {}: {} bytes to write, wait for EPOLLOUT again", m_sockfd, m_remainBytes);
            Utils::modepfd(s_epfd, m_sockfd, EPOLLOUT | EPOLLET | EPOLLONESHOT | EPOLLRDHUP);
            return;
        } else {
            // 对方关闭连接或其他错误
            s_logger->warn("[client] socket {}: writev error {}, connection closed", m_sockfd, strerror(errno));
            close();
            return;
        }
    }

    // 如果写入完毕，则释放文件引用并等待EPOLLIN事件
    s_logger->trace("[client] socket {}: write done", m_sockfd);
    m_fcont.reset();
    if (!m_keep_connection)
    {
        close();
    }
    Utils::modepfd(s_epfd, m_sockfd, EPOLLIN | EPOLLET | EPOLLONESHOT | EPOLLRDHUP);
}

bool HTTPClientTask::read()
{
    // 由于epoll使用了ET模式，因此这里需要循环读取直到recv返回-1
    if (m_readIdx >= m_readBuf.size())
        return false;

    for (;;)
    {
        int ret = recv(m_sockfd, m_readBuf.data() + m_readIdx, m_readBuf.size() - m_readIdx, 0);
        if (ret == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            s_logger->error("[client] socket {}: client error, recv return -1", m_sockfd);
            return false;
        }
        else if (ret == 0)
        {
            s_logger->error("[client] socket {}: recv return 0, connection closed by peer", m_sockfd);
            return false;
        }
        m_readIdx += ret;
        s_logger->trace("[client] socket {}: read {} bytes of data", m_sockfd, ret);
    }

    return true;
}
