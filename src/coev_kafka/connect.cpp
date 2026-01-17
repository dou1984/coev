#include "connect.h"

awaitable<int> Connect::ReadFull(std::string &buf, size_t n)
{
    assert(buf.size() == n);
    auto res = n;
    while (*this && res > 0)
    {
        auto r = co_await recv(buf.data() + (n - res), res);
        if (r == INVALID)
        {           
            LOG_ERR("ReadFull failed %d %s", errno, strerror(errno));
            co_return ErrIOEOF;
        }
        else if (r == 0)
        {
            LOG_ERR("ReadFull connection closed");
            co_return ErrIOEOF;
        }
        res -= r;
    }
    co_return ErrNoError;
}
static const char * LOG_HEX = "0123456789abcdef";
static std::string to_hex(const std::string &data)
{
    std::string res;
    res.reserve(data.size() * 2);  
    for (unsigned char c : data)
    {
        res += LOG_HEX[c >> 4];
        res += LOG_HEX[c & 0xf];
    }
    return res;
}
awaitable<int> Connect::Write(const std::string &buf)
{
    auto res = buf.size();
    while (*this && res > 0)
    {
        auto hex = to_hex(buf);
        LOG_CORE("%.*s", (int)hex.size(), hex.data());
        auto r = co_await send(buf.data() + (buf.size() - res), res);
        if (r == INVALID)
        {            
            LOG_ERR("Write failed %d %s", errno, strerror(errno));
            co_return ErrIOEOF;
        }
        else if (r == 0)
        {
            LOG_ERR("Write connection closed");
            co_return ErrIOEOF;
        }
        res -= r;
    }
    co_return ErrNoError;
}
awaitable<int> Connect::Dial(const char *addr, int port)
{   
    auto _fd = co_await base::connect(addr, port);
    if (_fd == INVALID)
    {
        LOG_ERR("Dial failed %d %s", errno, strerror(errno));
        co_return ErrIOEOF;
    }
    LOG_CORE("Dial %s:%d", addr, port);
    m_opened = true;
    co_return _fd;
}
int Connect::Close()
{
    if (m_opened)
    {
        m_opened = false;
    }
    return close();
}
Connect::operator bool() const
{
    return base::__valid() && m_opened;
}

