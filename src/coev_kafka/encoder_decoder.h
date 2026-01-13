
#pragma once
#include <vector>
#include <memory>
#include <cstdint>
#include "metrics.h"

struct packetDecoder;
struct packetEncoder;

struct PacketEncodingError
{
    std::string m_message;
};

struct IEncoder
{
    virtual ~IEncoder() = default;
    virtual int encode(packetEncoder &pe) = 0;
};

struct HEncoder : IEncoder
{
    virtual int16_t header_version() const = 0;
};
struct versionedEncoder
{
    virtual ~versionedEncoder() = default;
    virtual int encode(packetEncoder &pe, int16_t version) = 0;
};

struct IDecoder
{
    virtual ~IDecoder() = default;
    virtual int decode(packetDecoder &pd) = 0;
};

struct versionedDecoder
{
    virtual ~versionedDecoder() = default;
    virtual int decode(packetDecoder &pd, int16_t version) = 0;
};

struct flexible_version
{
    virtual ~flexible_version() = default;
    virtual bool is_flexible_version(int16_t version) = 0;
    virtual bool is_flexible() = 0;
};

int encode(std::shared_ptr<IEncoder> e, std::string &out, std::shared_ptr<metrics::Registry> metricRegistry);
int decode(const std::string &buf, std::shared_ptr<IDecoder> in, std::shared_ptr<metrics::Registry> metricRegistry);
int versionedDecode(const std::string &buf, const std::shared_ptr<versionedDecoder> &in, int16_t version, std::shared_ptr<metrics::Registry> &metricRegistry);
int magicValue(packetDecoder &pd, int8_t &magic);

int prepareFlexibleDecoder(packetDecoder *pd, std::shared_ptr<versionedDecoder> req, int16_t version);
int prepareFlexibleEncoder(packetEncoder *pe, std::shared_ptr<IEncoder> req);
std::shared_ptr<packetDecoder> downgradeFlexibleDecoder(std::shared_ptr<packetDecoder> pd);
