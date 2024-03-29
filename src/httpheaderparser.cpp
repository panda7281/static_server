#include "httpheaderparser.h"

HTTPHeaderParser::HTTPHeaderParser()
    : m_buf(nullptr), m_parsePos(0), m_readPos(0), m_lineStartPos(0), m_headerFieldValueStartPos(0),
      m_parseStatus(HTTPHeaderParser::_InternalStatus::CHECK_STARTLINE), m_parseLineStatus(HTTPHeaderParser::_InternalLineStatus::LINE_OK)
{
    m_respondHeader.reserve(128);
}

HTTPHeaderParser::~HTTPHeaderParser()
{
}

void HTTPHeaderParser::reset()
{
    // 重置状态
    m_parsePos = 0;
    m_readPos = 0;
    m_lineStartPos = 0;
    m_headerFieldValueStartPos = 0;
    m_parseStatus = HTTPHeaderParser::_InternalStatus::CHECK_STARTLINE;
    m_parseLineStatus = HTTPHeaderParser::_InternalLineStatus::LINE_OK;
}

void HTTPHeaderParser::setBuffer(char *buf)
{
    m_buf = buf;
    reset();
}

HTTPHeaderParser::Status HTTPHeaderParser::parseRequest(int read_pos)
{
    m_readPos = read_pos;
    while ((m_parseLineStatus = parseLine()) == HTTPHeaderParser::_InternalLineStatus::LINE_OK)
    {
        switch (m_parseStatus)
        {
        case HTTPHeaderParser::_InternalStatus::CHECK_STARTLINE:
            // 格式化第一行
            if (!parseStartLine())
            {
                return HTTPHeaderParser::Status::ERROR;
            }
            else
            {
                m_parseStatus = HTTPHeaderParser::_InternalStatus::CHECK_LINES;
            }
            break;
        case HTTPHeaderParser::_InternalStatus::CHECK_LINES:
            // 格式化后续的键值对，放进opt映射中，如果单行只有\r\n，则认为请求头格式化完成
            // 只有两个字符
            if (m_parsePos - m_lineStartPos == 1)
            {
                return HTTPHeaderParser::Status::DONE;
            }
            // 向opt添加键值对
            else if (m_parsePos > m_headerFieldValueStartPos && m_headerFieldValueStartPos > m_lineStartPos)
            {
                auto headerKey = std::string(m_buf + m_lineStartPos, m_headerFieldValueStartPos - m_lineStartPos - 2);
                auto headerVal = std::string(m_buf + m_headerFieldValueStartPos, m_parsePos - m_headerFieldValueStartPos - 1);
                m_requestHeader.opt[headerKey] = headerVal;
            }
            else
            {
                return HTTPHeaderParser::Status::ERROR;
            }
            break;
        default:
            break;
        }
        m_lineStartPos = m_parsePos + 1;
    }

    if (m_parseLineStatus == HTTPHeaderParser::_InternalLineStatus::LINE_OPEN)
    {
        return HTTPHeaderParser::Status::WORKING;
    }
    else
    {
        return HTTPHeaderParser::Status::ERROR;
    }
}

HTTPHeaderParser::RequestHeader *HTTPHeaderParser::getRequestHeader()
{
    return &m_requestHeader;
}

std::string *HTTPHeaderParser::getRespondHeader(std::string version, StatusCode code, std::optional<KVMap> opt)
{
    m_respondHeader.clear();
    // 第一行
    m_respondHeader += version;
    m_respondHeader += ' ';
    auto codestr = status2str(code);
    m_respondHeader += codestr;
    m_respondHeader += "\r\n";
    // 后续的键值对
    if (code != StatusCode::OK)
    {
        m_respondHeader += "Content-Length: ";
        m_respondHeader += std::to_string(codestr.size());
        m_respondHeader += "\r\n";
        // 最后的空行
        m_respondHeader += "\r\n";
        // 如果不是200 OK，那么加上一个基本的错误码
        m_respondHeader += codestr;
    }
    else
    {
        if (opt)
        {
            for (const auto &[k, v] : opt.value())
            {
                m_respondHeader += k;
                m_respondHeader += ": ";
                m_respondHeader += v;
                m_respondHeader += "\r\n";
            }
        }
        // 最后的空行
        m_respondHeader += "\r\n";
    }
    
    return &m_respondHeader;
}

HTTPHeaderParser::_InternalLineStatus HTTPHeaderParser::parseLine()
{
    char temp;
    for (; m_parsePos < m_readPos; m_parsePos++)
    {
        temp = m_buf[m_parsePos];
        // 分以下情况
        // 如果当前字符是本行中第一个出现的冒号，则将位置+2作为HeaderFieldValue的起点
        if (temp == ':')
        {
            if (m_headerFieldValueStartPos <= m_lineStartPos)
                m_headerFieldValueStartPos = m_parsePos + 2;
        }
        // 如果当前字符是\n，则检查上一个字符是不是\r
        else if (temp == '\n')
        {
            if (m_parsePos >= 1 && m_buf[m_parsePos - 1] == '\r')
            {
                m_buf[m_parsePos] = '\0';
                m_buf[m_parsePos - 1] = '\0';
                return HTTPHeaderParser::_InternalLineStatus::LINE_OK;
            }
            else
            {
                return HTTPHeaderParser::_InternalLineStatus::LINE_BAD;
            }
        }
    }

    return HTTPHeaderParser::_InternalLineStatus::LINE_OPEN;
}

bool HTTPHeaderParser::parseStartLine()
{
    std::string startLine(m_buf);
    // 找到第一个分隔符
    auto pos1 = startLine.find(' ');
    if (pos1 == std::string::npos)
        return false;
    std::string methodstr(m_buf, pos1);
    auto method = str2method(methodstr);
    if (method == HTTPHeaderParser::Method::UNKNOWN)
        return false;
    m_requestHeader.method = method;
    // 找到第二个分隔符，中间的内容为path
    auto pos2 = startLine.find(' ', pos1 + 1);
    if (pos2 == std::string::npos)
        return false;
    // 预处理，path只能以 以http://开头，则将其移除
    auto pathstr = std::string(startLine, pos1 + 1, pos2 - pos1 - 1);
    if (pathstr.starts_with("http://"))
    {
        pathstr = std::string(pathstr, 7);
    }
    else if (pathstr.starts_with("/"))
    {
        pathstr = std::string(pathstr, 1);
    }
    else
    {
        return false;
    }
    m_requestHeader.path = pathstr;
    // 剩下的部分为HTTP版本
    m_requestHeader.version = std::string(startLine, pos2 + 1);
    return true;
}

HTTPHeaderParser::Method HTTPHeaderParser::str2method(const std::string &str)
{
    if (str == "GET")
    {
        return HTTPHeaderParser::Method::GET;
    }
    else if (str == "POST")
    {
        return HTTPHeaderParser::Method::POST;
    }
    else if (str == "PUT")
    {
        return HTTPHeaderParser::Method::PUT;
    }
    else if (str == "DELETE")
    {
        return HTTPHeaderParser::Method::DELETE;
    }
    else if (str == "HEAD")
    {
        return HTTPHeaderParser::Method::HEAD;
    }
    else if (str == "TRACE")
    {
        return HTTPHeaderParser::Method::TRACE;
    }
    else if (str == "OPTIONS")
    {
        return HTTPHeaderParser::Method::OPTIONS;
    }
    else if (str == "CONNECT")
    {
        return HTTPHeaderParser::Method::CONNECT;
    }
    else if (str == "PATCH")
    {
        return HTTPHeaderParser::Method::PATCH;
    }
    else
    {
        return HTTPHeaderParser::Method::UNKNOWN;
    }
}

std::string HTTPHeaderParser::status2str(StatusCode status)
{
    std::string statusstr;
    switch (status)
    {
    case StatusCode::CONTINUE:
        statusstr = "100 Continue";
        break;
    case StatusCode::OK:
        statusstr = "200 OK";
        break;
    case StatusCode::MOVED_PERMANENTLY:
        statusstr = "301 Moved Permanently";
        break;
    case StatusCode::FOUND:
        statusstr = "302 Found";
        break;
    case StatusCode::NOT_MODIFIED:
        statusstr = "304 Not Modified";
        break;
    case StatusCode::TEMPORARY_REDIRECT:
        statusstr = "307 Temporary Redirect";
        break;
    case StatusCode::BAD_REQUEST:
        statusstr = "400 Bad Request";
        break;
    case StatusCode::UNAUTHORIZED:
        statusstr = "401 Unauthorized";
        break;
    case StatusCode::FORBIDDEN:
        statusstr = "403 Forbidden";
        break;
    case StatusCode::NOT_FOUND:
        statusstr = "404 Not Found";
        break;
    case StatusCode::PROXY_AUTHENTICATION_REQUIRED:
        statusstr = "407 Proxy Authentication Required";
        break;
    case StatusCode::INTERNAL_SERVER_ERROR:
        statusstr = "500 Internal Server Error";
        break;
    case StatusCode::SERVICE_UNAVAILABLE:
        statusstr = "503 Service Unavailable";
        break;
    default:
        break;
    }
    return statusstr;
}
