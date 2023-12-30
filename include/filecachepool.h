#pragma once

#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <fcntl.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <list>
#include <mutex>

class FileCacheItem
{
public:
    FileCacheItem(const std::string &path) : m_path(path), m_data(nullptr)
    {
        loadFile();
    }

    ~FileCacheItem()
    {
        if (m_data != nullptr)
        {
            munmap(m_data, m_fstat.st_size);
        }
    }

    const std::string &getPath() const
    {
        return m_path;
    }

    const void *getData() const
    {
        return m_data;
    }

    const struct stat *getStat() const
    {
        return &m_fstat;
    }

private:
    void loadFile()
    {
        if (::stat(m_path.c_str(), &m_fstat) < 0)
            return;
        if (!(m_fstat.st_mode & S_IROTH) || S_ISDIR(m_fstat.st_mode))
            return;
        int fd = open(m_path.c_str(), O_RDONLY);
        if (fd < 0)
            return;
        m_data = ::mmap(0, m_fstat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        ::close(fd);
        if (m_data == MAP_FAILED)
        {
            m_data = nullptr;
            return;
        }
    }

    std::string m_path;
    void *m_data;
    struct stat m_fstat;
};

class FileCachePool
{
public:
    struct FileContent
    {
        const void *data;
        const struct stat *fstat;
    };
    FileCachePool(off_t max_size, int max_item);
    ~FileCachePool();

    std::shared_ptr<FileCacheItem> getFile(const std::string &path);

private:
    off_t m_maxSize, m_currSize;
    int m_maxItem;

    std::unordered_map<std::string, std::shared_ptr<FileCacheItem>> m_cacheMap;
    std::list<std::string> m_cacheOrder; 
    std::mutex m_lock;
    
    void evict();
};
