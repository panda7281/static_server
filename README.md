# 静态资源服务器

## 架构

### 设计思路

使用reactor架构（有些地方称为半同步/半反应堆），主线程在监听socket和连接socket上调用`epoll_wait`，有事件发生时（新连接/读就绪/写就绪）将其添加至任务队列，工作线程从任务队列获取任务并处理

### 核心组件

- 线程池：负责从socket上读取数据，格式化请求头/响应头，并将响应写入socket，使用`std::thread`实现
- 文件缓存池：使用LRU（Least Recently Used）策略缓存请求的文件避免频繁进行磁盘I/O
- 定时器：定时处理非活动连接，使用时间轮实现

## 编译

### 安装依赖项

- [spdlog](https://github.com/gabime/spdlog): 提供日志功能
- [json](https://github.com/nlohmann/json): 用于读取配置文件

### 使用CMake生成二进制文件

```sh
cd static_server/
cmake -B build -S .
cmake --build build --config Release
```

## 配置

服务器的配置文件位于`运行目录/config/config.json`, 内容如下

```json
{
    "host": "0.0.0.0",
    "port": 12345,
    "root": "www",
    "loglevel": 1,
    "backlog": 100,
    "timeout": 10,
    "threadpool": {
        "workers" : 8,
        "maxtask" : 20000
    },
    "cachepool": {
        "maxsize" : 1073741824,
        "maxitem" : 65536
    },
    "timer": {
        "granularity": 10,
        "interval": 1
    }
}
```

## 运行

```sh
cd static_server/
./build/StaticServer
```

## 压力测试结果

**详见test/result.txt**

测试环境: i5-1240p@12C16T 16g wsl2 ubuntu 22.04

测试工具: [wrk](https://github.com/wg/wrk/tree/master)

### 高压环境

使用脚本wrk/stress.lua, 8 worker threads, backlog=100, 1000 connections

平均延迟: 4.12ms

请求频率: 245697.17req/s

吞吐量: 127.94MB/s

### 模拟真实情况

使用脚本wrk/simu.lua, 8 worker threads, backlog=100, 1000 connections

请求之间添加10ms-50ms的随机延迟，每次随机请求50张jpg中的一个

平均延迟: 3.73ms

请求频率: 29395.20req/s

吞吐量: 6.76GB/s