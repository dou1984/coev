
#pragma once
#include <vector>
#include <memory>
#include <cstdint>
#include <cassert>

struct packet_type
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

struct packet_decoder;
struct packet_encoder;

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
    virtual int encode(packet_encoder &pe) const = 0;
};

struct HEncoder : IEncoder
{
    virtual int16_t header_version() const = 0;
};
struct VEncoder
{
    virtual ~VEncoder() = default;
    virtual int encode(packet_encoder &pe, int16_t version) const = 0;
};

struct IDecoder
{
    virtual ~IDecoder() = default;
    virtual int decode(packet_decoder &pd) = 0;
};

struct VDecoder
{
    virtual ~VDecoder() = default;
    virtual int decode(packet_decoder &pd, int16_t version) = 0;
};

int encode(IEncoder &e, std::string &out);
int decode(const std::string &buf, IDecoder &in);
int versioned_decode(const std::string &buf, VDecoder &in, int16_t version);
int magic_value(packet_decoder &pd, int8_t &magic);

int prepare_flexible_decoder(packet_decoder &pd, VDecoder &req, int16_t version);
int prepare_flexible_encoder(packet_encoder &pe, IEncoder &req);
std::shared_ptr<packet_decoder> downgrade_flexible_decoder(std::shared_ptr<packet_decoder> pd);
