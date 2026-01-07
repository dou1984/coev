#include <iostream>
#include <coev/coev.h>
#include <coev_nghttp/co_nghttp2.h>

using namespace coev;

awaitable<void> co_request()
{
    nghttp2::client parser(nullptr);
    int fd = co_await parser.connect("157.148.69.186");

    if (fd < 0)
    {
        std::cout << "connect failed" << std::endl;
        co_return;
    }
    const char w[] = "GET / HTTP/1.1\r\nHost: www.baidu.com\r\n\r\n";
    co_await parser.send(w, sizeof(w));
}

awaitable<void> co_response()
{
    nghttp2::client parser(nullptr);
    int fd = co_await parser.connect("www.baidu.com");

    if (fd < 0)
    {
        std::cout << "connect failed" << std::endl;
        co_return;
    }
    const char w[] = "GET / HTTP/1.1\r\nHost: www.baidu.com\r\n\r\n";
    co_await parser.send(w, sizeof(w));
}
int main()
{
    runnable::instance()
        .start(co_request)
        .join();
    return 0;
}