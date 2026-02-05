#include "connect.h"

static const char *LOG_HEX = "0123456789abcdef";
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
Connect::Connect() : m_state(CLOSED)
{
}

Connect::~Connect()
{    
}
awaitable<int> Connect::ReadFull(std::string &buf, size_t n)
{
    assert(buf.size() == n);
    auto res = n;
    while (__valid() && res > 0)
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
    auto hex = to_hex(buf);
    LOG_CORE("ReadFull received: %.*s", (int)hex.size(), hex.data());
    co_return ErrNoError;
}

awaitable<int> Connect::Write(const std::string &buf)
{
    auto res = buf.size();
    while (__valid() && res > 0)
    {
        auto hex = to_hex(buf);
        LOG_CORE("Write sending: %.*s", (int)hex.size(), hex.data());
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
    m_state = OPENING;
    auto _fd = co_await base::connect(addr, port);
    if (_fd == INVALID)
    {
        m_state = CLOSED;
        LOG_CORE("Dial failed %d %s", errno, strerror(errno));
        co_return INVALID;
    }
    LOG_CORE("Dial success %s:%d", addr, port);
    m_state = OPENED;
    co_return _fd;
}
int Connect::Close()
{
    m_state = CLOSED;
    return close();
}
bool Connect::IsClosed() const
{
    return m_state == CLOSED;
}
bool Connect::IsOpening() const
{
    return m_state == OPENING;
}
bool Connect::IsOpened() const
{
    return m_state == OPENED;
}
int Connect::State() const
{
    return m_state;
}