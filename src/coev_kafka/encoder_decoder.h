
#pragma once
#include <vector>
#include <memory>
#include <cstdint>
#include "metrics.h"

struct PDecoder;
struct PEncoder;

struct PacketEncodingError
{
    std::string m_message;
};

struct IEncoder
{
    virtual ~IEncoder() = default;
    virtual int encode(PEncoder &pe) = 0;
};

struct HEncoder : IEncoder
{
    virtual int16_t headerVersion() const = 0;
};
struct VEncoder
{
    virtual ~VEncoder() = default;
    virtual int encode(PEncoder &pe, int16_t version) = 0;
};

struct IDecoder
{
    virtual ~IDecoder() = default;
    virtual int decode(PDecoder &pd) = 0;
};

struct VDecoder
{
    virtual ~VDecoder() = default;
    virtual int decode(PDecoder &pd, int16_t version) = 0;
};

struct flexible_version
{
    virtual ~flexible_version() = default;
    virtual bool is_flexible_version(int16_t version) = 0;
    virtual bool is_flexible() = 0;
};

int encode(std::shared_ptr<IEncoder> e, std::string &out, std::shared_ptr<metrics::Registry> metricRegistry);
int decode(const std::string &buf, std::shared_ptr<IDecoder> in, std::shared_ptr<metrics::Registry> metricRegistry);
int versionedDecode(const std::string &buf, const std::shared_ptr<VDecoder> &in, int16_t version, std::shared_ptr<metrics::Registry> &metricRegistry);
int magicValue(PDecoder &pd, int8_t &magic);

int prepareFlexibleDecoder(PDecoder *pd, std::shared_ptr<VDecoder> req, int16_t version);
int prepareFlexibleEncoder(PEncoder *pe, std::shared_ptr<IEncoder> req);
std::shared_ptr<PDecoder> downgradeFlexibleDecoder(std::shared_ptr<PDecoder> pd);
