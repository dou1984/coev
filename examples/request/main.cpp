#include <iostream>
#include <coev/coev.h>
#include <co_nghttp/NghttpSession.h>

coev::awaitable<int> co_request()
{
    http2::Nghttprequest parser;
    int fd = co_await parser.connect("157.148.69.80");

    if (fd < 0)
    {
        std::cout << "connect failed" << std::endl;
        co_return -1;
    }
    co_await parser.send("GET / HTTP/1.1\r\nHost: www.baidu.com\r\n\r\n");
    co_return 0;
}

coev::awaitable<int> co_response()
{
    http2::Nghttprequest parser;
    int fd = co_await parser.connect("www.baidu.com");

    if (fd < 0)
    {
        std::cout << "connect failed" << std::endl;
        co_return -1;
    }
    co_await parser.send()
}
int main()
{
    coev::runnable::instance(co_request).join();
    return 0;
}