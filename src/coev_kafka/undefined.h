#pragma once
#include <chrono>
#include <cstdint>
#include <memory>
#include <coev/coev.h>
#include "access_token.h"

struct Client;
struct push_decoder;
struct AccessTokenProvider
{
    virtual int Token(std::shared_ptr<AccessToken> &) = 0;
};
struct ProxyDialer : std::enable_shared_from_this<ProxyDialer>
{
    std::chrono::milliseconds m_timeout;
    std::chrono::milliseconds m_keepalive;
    sockaddr_in m_local_addr;
    ProxyDialer(std::chrono::milliseconds timeout, std::chrono::milliseconds keepAlive, const sockaddr_in &localAddr)
        : m_timeout(timeout), m_keepalive(keepAlive), m_local_addr(localAddr)
    {
    }
    std::shared_ptr<Client> Dial(const std::string &, const std::string &)
    {
        return {nullptr};
    }
};

struct Ticket
{
};
namespace net
{
    inline std::string JoinHostPort(const std::string host, int32_t port)
    {
        return host + ":" + std::to_string(port);
    }
    inline std::pair<std::string, int32_t> SplitHostPort(const std::string &addr)
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
    coev::co_task m_task;
    coev::co_channel<bool> ch;
};
