#pragma once

#include <vector>
#include <map>
#include <string>
#include <optional>

class HTTPHeaderParser
{
public:
    using KVMap = std::map<std::string, std::string>;

    enum class Status {
        WORKING,
        ERROR,
        DONE
    };

    enum class Method
    {
        GET,
        POST,
        PUT,
        DELETE,
        HEAD, 
        TRACE,
        OPTIONS,
        CONNECT,
        PATCH,
        UNKNOWN
    };

    enum class StatusCode
    {
        CONTINUE,
        OK,
        MOVED_PERMANENTLY,
        FOUND,
        NOT_MODIFIED,
        TEMPORARY_REDIRECT,
        BAD_REQUEST,
        UNAUTHORIZED,
        FORBIDDEN,    
        NOT_FOUND,
        PROXY_AUTHENTICATION_REQUIRED,
        INTERNAL_SERVER_ERROR,
        SERVICE_UNAVAILABLE
    };

    struct RequestHeader
    {
        Method method;
        std::string version;
        std::string path;
        KVMap opt;
    };

    HTTPHeaderParser();
    ~HTTPHeaderParser();
    // 重置全部内部状态，但不更改m_buf指向的地址
    void reset();
    // 更改m_buf指向的地址并重置全部内部状态
    void setBuffer(char* buf);
    // 根据当前读取到的位置格式化HTTP请求头
    // 返回WORKING代表进行中，数据不完整
    // 返回DONE代表格式化完成
    // 返回ERROR代表请求头中有错误
    Status parseRequest(int read_pos);
    // 获取指向请求头的指针
    RequestHeader* getRequestHeader();
    // 根据返回状态生成响应头，返回字符串指针
    std::string* getRespondHeader(std::string version, StatusCode code, std::optional<KVMap> opt);

private:
    enum class _InternalStatus
    {
        CHECK_STARTLINE,
        CHECK_LINES
    };

    enum class _InternalLineStatus
    {
        LINE_OPEN,
        LINE_OK,
        LINE_BAD
    };

    char* m_buf;
    int m_parsePos;
    int m_readPos;
    int m_lineStartPos;
    int m_headerFieldValueStartPos;
    _InternalStatus m_parseStatus;
    _InternalLineStatus m_parseLineStatus;
    RequestHeader m_requestHeader;
    std::string m_respondHeader;

    _InternalLineStatus parseLine();
    bool parseStartLine();
    
    static Method str2method(const std::string& str);
    static std::string status2str(StatusCode status);
};
