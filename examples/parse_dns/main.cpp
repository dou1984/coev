#include <coev/coev.h>
#include <coev_dns/parse_dns.h>
#include <coev/log.h>

using namespace coev;

awaitable<void> co_parse_dns()
{
    std::string url = "www.baidu.com";
    // std::string url = "xxxxxxxx";
    std::string addr = "";
    auto r = co_await parse_dns(url, addr);
    if (r != 0)
    {
        LOG_ERR("parse_dns error \n");
        co_return;
    }
    LOG_INFO("baidu -> %s\n", addr.c_str());

    cosys::stop();
    co_return;
}

int main()
{

    set_log_level(LOG_LEVEL_CORE);

    runnable::instance().start(co_parse_dns).join();

    return 0;
}