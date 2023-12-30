#include "utils.h"

namespace Utils
{
    int addepfd(int epfd, int fd, uint32_t flag)
    {
        epoll_event ev;
        ev.data.fd = fd;
        ev.events = flag;
        return epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    }
    int delepfd(int epfd, int fd)
    {
        return epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
    }
    int modepfd(int epfd, int fd, uint32_t flag)
    {
        epoll_event ev;
        ev.data.fd = fd;
        ev.events = flag;
        return epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
    }
    int setfdnonblocking(int fd)
    {
        int fl_org = fcntl(fd, F_GETFL);
        if (fl_org < 0)
            return -1;
        return fcntl(fd, F_SETFL, fl_org | O_NONBLOCK);
    }
    int setreusefd(int fd, bool on)
    {
        int optval = on ? 1 : 0;
        return ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
               &optval, sizeof(optval));
    }
    int setsighandler(int sig, sighandler_t handler)
    {
        signal(sig, handler);
        return 0;
    }
    std::string addr2str(sockaddr_in& addr)
    {
        char buf[32];
        inet_ntop(AF_INET, reinterpret_cast<void*>(&(addr.sin_addr)), buf, static_cast<socklen_t>(sizeof(buf)));
        std::string addrstr(buf);
        addrstr += ':';
        addrstr += std::to_string(ntohs(addr.sin_port));
        return addrstr;
    }
}