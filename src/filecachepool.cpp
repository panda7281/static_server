#include "filecachepool.h"

FileCachePool::FileCachePool(off_t max_size, int max_item)
{
    m_maxItem = std::max(max_item, 0);
    m_maxSize = std::max(max_size, static_cast<off_t>(0));
    m_currSize = 0;
}

FileCachePool::~FileCachePool()
{
    
}

std::shared_ptr<FileCacheItem> FileCachePool::getFile(const std::string &path)
{
    std::scoped_lock locker(m_lock);
    auto it = m_cacheMap.find(path);
    if (it != m_cacheMap.end())
    {
        // 在缓存中找到了文件
        // 更新order
        // 检查文件的更新时间和大小与缓存中的是否一致，如果不一致，则删除旧文件
        m_cacheOrder.remove(path);
        if (fileConsistencyCheck(path, *m_cacheMap[path]->getStat()))
        {
            // 通过了一致性检查
            m_cacheOrder.push_front(path);
            return it->second;
        }
        else
        {
            m_cacheMap.erase(path);
        }

    }
    // 在缓存中没有找到文件
    // 创建新的缓存项
    auto newFileCache = std::make_shared<FileCacheItem>(path);
    if (!newFileCache->getData())
    {
        // 文件读取失败，返回一个空指针
        return std::shared_ptr<FileCacheItem>();
    }
    // 添加到map和order
    m_cacheMap[path] = newFileCache;
    m_cacheOrder.push_front(path);
    // 更新大小
    m_currSize += newFileCache->getStat()->st_size;
    // 清除多余的缓存项
    evict();
    return newFileCache;
}

off_t FileCachePool::getCurrentSize()
{
    return m_currSize;
}

int FileCachePool::getCurrentItemCount()
{
    return m_cacheOrder.size();
}

void FileCachePool::evict()
{
    while (m_currSize > m_maxSize || m_cacheMap.size() > m_maxItem)
    {
        auto& curr_rm_path = m_cacheOrder.back();
        auto curr_rm_size = m_cacheMap[curr_rm_path]->getStat()->st_size;
        m_cacheMap.erase(curr_rm_path);
        m_cacheOrder.pop_back();
        m_currSize -= curr_rm_size;
    }
}

bool FileCachePool::compareTimeSpec(const timespec &t1, const timespec &t2)
{
    return t1.tv_nsec == t2.tv_nsec;
}

bool FileCachePool::fileConsistencyCheck(const std::string &path, const struct stat& old_fstat)
{
    struct stat fstat;
    if (::stat(path.c_str(), &fstat) < 0)
        return false;

    if (!compareTimeSpec(fstat.st_mtim, old_fstat.st_mtim) || !compareTimeSpec(fstat.st_atim, old_fstat.st_atim) || !compareTimeSpec(fstat.st_ctim, old_fstat.st_ctim) || fstat.st_size != old_fstat.st_size)
    {
        return false;
    }

    return true;
}
