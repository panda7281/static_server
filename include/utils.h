#pragma once

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <signal.h>
#include <fcntl.h>
#include <signal.h>
#include <string>

namespace Utils
{
    int addepfd(int epfd, int fd, uint32_t flag);
    int delepfd(int epfd, int fd);
    int modepfd(int epfd, int fd, uint32_t flag);
    int setfdnonblocking(int fd);
    int setreusefd(int fd, bool on);
    int setsighandler(int signal, sighandler_t handler);
    std::string addr2str(sockaddr_in& addr);
}