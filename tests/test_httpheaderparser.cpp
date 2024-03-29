#include <catch2/catch_all.hpp>
#include "httpheaderparser.h"
#include <string>

TEST_CASE("HTTP Header Parser", "[basic]")
{
    HTTPHeaderParser parser;
    char buffer[1024];
    memset(buffer, 0, sizeof(char)*1024);
    parser.setBuffer(buffer);

    std::string request_header = "GET /index.html HTTP/1.1\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36\r\nHost: www.bilibili.com\r\nConnection: keep-alive\r\n\r\n";
    memcpy(buffer, request_header.c_str(), request_header.size());
    
    auto status = parser.parseRequest(request_header.size());
    REQUIRE(status == HTTPHeaderParser::Status::DONE);
    auto header = parser.getRequestHeader();
    REQUIRE(header->method == HTTPHeaderParser::Method::GET);
    REQUIRE(header->path == "index.html");
    REQUIRE(header->version == "HTTP/1.1");
    REQUIRE(header->opt["User-Agent"] == "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
    REQUIRE(header->opt["Host"] == "www.bilibili.com");
    REQUIRE(header->opt["Connection"] == "keep-alive");

}

TEST_CASE("HTTP Header Parser", "[incremental]")
{
    HTTPHeaderParser parser;
    char buffer[1024];
    memset(buffer, 0, sizeof(char)*1024);
    parser.setBuffer(buffer);

    std::string request_header = "GET /index.html HTTP/1.1\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36\r\nHost: www.bilibili.com\r\nConnection: keep-alive\r\n\r\n";
    memcpy(buffer, request_header.c_str(), request_header.size());
    
    auto status = parser.parseRequest(10);
    REQUIRE(status == HTTPHeaderParser::Status::WORKING);
    status = parser.parseRequest(20);
    REQUIRE(status == HTTPHeaderParser::Status::WORKING);
    status = parser.parseRequest(25);
    REQUIRE(status == HTTPHeaderParser::Status::WORKING);
    status = parser.parseRequest(request_header.size());
    REQUIRE(status == HTTPHeaderParser::Status::DONE);

    auto header = parser.getRequestHeader();
    REQUIRE(header->method == HTTPHeaderParser::Method::GET);
    REQUIRE(header->path == "index.html");
    REQUIRE(header->version == "HTTP/1.1");
    REQUIRE(header->opt["User-Agent"] == "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
    REQUIRE(header->opt["Host"] == "www.bilibili.com");
    REQUIRE(header->opt["Connection"] == "keep-alive");
}

TEST_CASE("HTTP Header Parser", "[respond parser]")
{
    HTTPHeaderParser parser;
    HTTPHeaderParser::KVMap opt;
    opt["Connection"] = "keep-alive";
    opt["Content-Type"] = "text/html; charset=utf-8";
    auto respond = parser.getRespondHeader("HTTP/1.1", HTTPHeaderParser::StatusCode::OK, opt);
    REQUIRE(*respond == "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nContent-Type: text/html; charset=utf-8\r\n\r\n");
}