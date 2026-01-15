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
            // Handle non-blocking I/O errors
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS)
            {
                // Retry the operation
                continue;
            }
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
static std::string to_hex(const std::string &data)
{
    std::string res;
    for (unsigned char c : data)
    {
        res += "0123456789abcdef"[c >> 4];
        res += "0123456789abcdef"[c & 0xf];
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
            // Handle non-blocking I/O errors
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS)
            {
                // Retry the operation
                continue;
            }
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
    // Initialize the event loop before calling connect
    InitLoop();
    
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

void Connect::InitLoop()
{
    // Explicitly initialize the event loop pointer with the current thread's event loop
    auto loop = coev::cosys::data();
    LOG_CORE("InitLoop: Current loop is %p, base::m_loop is %p", loop, base::m_loop);
    if (base::m_loop == nullptr) {
        LOG_CORE("InitLoop: Setting base::m_loop to %p", loop);
        // Access the m_loop pointer through the base class
        base::m_loop = loop;
        LOG_CORE("InitLoop: After setting, base::m_loop is %p", base::m_loop);
    }
    // Ensure the underlying io_connect object is properly initialized
    if (base::m_fd == INVALID) {
        LOG_CORE("InitLoop: Initializing underlying io_connect object");
        // Re-initialize the io_connect object
        base::__init_connect();
        base::m_type |= IO_CLIENT | IO_TCP;
    }
}