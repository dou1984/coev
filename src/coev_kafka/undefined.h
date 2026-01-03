#pragma once
#include <chrono>
#include <cstdint>
#include <memory>
#include <coev/coev.h>
#include "access_token.h"

struct Client;
struct pushDecoder;
struct AccessTokenProvider
{
    virtual int Token(std::shared_ptr<AccessToken> &) = 0;
};
struct ProxyDialer : std::enable_shared_from_this<ProxyDialer>
{
    std::chrono::milliseconds Timeout;
    std::chrono::milliseconds KeepAlive;
    sockaddr_in LocalAddr;
    ProxyDialer(std::chrono::milliseconds timeout, std::chrono::milliseconds keepAlive, const sockaddr_in &localAddr)
        : Timeout(timeout), KeepAlive(keepAlive), LocalAddr(localAddr)
    {
    }
    std::shared_ptr<Client> Dial(const std::string &, const std::string &)
    {
        return {nullptr};
    }
};

namespace buf
{
    struct Conn
    {
        auto ConnectionState()
        {
            return 0;
        }
    };

}

struct Hash32
{
    void Reset()
    {
    }
    void Update(const char *data, size_t size)
    {
    }
    uint32_t Final()
    {
        return 0;
    }
};

namespace fnv
{
    std::shared_ptr<Hash32> New32a()
    {
        return std::make_shared<Hash32>();
    }
};

struct Timer
{
};

struct none
{
};

struct Breaker
{
};
struct Ticket
{
};
namespace net
{
    std::string JoinHostPort(const std::string host, int32_t port)
    {
        return host + ":" + std::to_string(port);
    }
    std::pair<std::string, int32_t> SplitHostPort(const std::string &addr)
    {
        auto pos = addr.find(":");
        if (pos == std::string::npos)
        {
            return std::make_pair(addr, 0);
        }
        return std::make_pair(addr.substr(0, pos), std::stoi(addr.substr(pos + 1)));
    }
}

struct EncryptionKey
{
};
struct PrincipalName
{
};
struct Context
{
    auto &get()
    {
        return ch;
    }
    void done()
    {
        ch.set(true);
    }
    coev::co_channel<bool> ch;
};
inline uint32_t *crc32_ieee_table()
{
    static uint32_t crc32_ieee_table_[256];
    return crc32_ieee_table_;
}
