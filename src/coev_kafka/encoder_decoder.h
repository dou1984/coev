
#pragma once
#include <vector>
#include <memory>
#include <cstdint>
#include <cassert>

struct packetType
{
    int m_flexible = 0;
    bool isFixed()
    {
        return m_flexible == 0;
    }
    bool isFlexible()
    {
        return m_flexible > 0;
    }
    void pushFlexible()
    {
        m_flexible++;
    }
    void popFlexible()
    {
        assert(m_flexible > 0);
        m_flexible--;
    }
};

struct packetDecoder;
struct packetEncoder;

struct PacketEncodingError
{
    std::string m_message;
};

struct flexible_version
{
    virtual ~flexible_version() = default;
    virtual bool is_flexible_version(int16_t version) const = 0;
    virtual bool is_flexible() const = 0;
};
struct IEncoder
{
    virtual ~IEncoder() = default;
    virtual int encode(packetEncoder &pe) const = 0;
};

struct header_encoder : IEncoder
{
    virtual int16_t header_version() const = 0;
};
struct versioned_encoder
{
    virtual ~versioned_encoder() = default;
    virtual int encode(packetEncoder &pe, int16_t version) const = 0;
};

struct IDecoder
{
    virtual ~IDecoder() = default;
    virtual int decode(packetDecoder &pd) = 0;
};

struct versioned_decoder
{
    virtual ~versioned_decoder() = default;
    virtual int decode(packetDecoder &pd, int16_t version) = 0;
};

int encode(IEncoder &e, std::string &out);
int decode(const std::string &buf, IDecoder &in);
int versionedDecode(const std::string &buf, versioned_decoder &in, int16_t version);
int magicValue(packetDecoder &pd, int8_t &magic);

int prepareFlexibleDecoder(packetDecoder &pd, versioned_decoder &req, int16_t version);
int prepareFlexibleEncoder(packetEncoder &pe, IEncoder &req);
std::shared_ptr<packetDecoder> downgradeFlexibleDecoder(std::shared_ptr<packetDecoder> pd);
