
#include <string>
#include "encoder_decoder.h"
#include "prep_encoder.h"
#include "real_encoder.h"
#include "request.h"
#include "errors.h"
#include "packet_decoding_error.h"

int encode(std::shared_ptr<IEncoder> e, std::string &out, std::shared_ptr<metrics::Registry> metricRegistry)
{
    if (!e)
    {
        return ErrNoError;
    }

    auto prepEnc = std::make_shared<prepEncoder>();
    if (prepareFlexibleEncoder(prepEnc.get(), e) != ErrNoError)
    {
        throw PacketEncodingError{"encoding failed"};
    }

    if (prepEnc->m_length < 0 || prepEnc->m_length > static_cast<int>(MaxRequestSize))
    {
        throw PacketEncodingError{"invalid request size (" + std::to_string(prepEnc->m_length) + ")"};
    }

    auto realEnc = std::make_shared<realEncoder>();
    realEnc->m_raw.resize(prepEnc->m_length);
    realEnc->m_metric_registry = metricRegistry;
    if (prepareFlexibleEncoder(realEnc.get(), e) != ErrNoError)
    {
        throw PacketEncodingError{"encoding failed"};
    }

    out.insert(out.end(), realEnc->m_raw.begin(), realEnc->m_raw.end());
    return ErrNoError;
}
int decode(const std::string &buf, std::shared_ptr<IDecoder> in, std::shared_ptr<metrics::Registry> metricRegistry)
{
    if (buf.empty())
    {
        return ErrNoError;
    }

    realDecoder helper;
    helper.m_raw = buf;
    helper.m_metric_registry = metricRegistry;

    if (in->decode(helper) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (helper.m_offset != static_cast<int>(buf.size()))
    {
        throw PacketDecodingError{"invalid length: buf=" + std::to_string(buf.size()) + " decoded=" + std::to_string(helper.m_offset)};
    }

    return ErrNoError;
}

int versionedDecode(const std::string &buf, const std::shared_ptr<VDecoder> &in, int16_t version, std::shared_ptr<metrics::Registry> &metricRegistry)
{
    if (buf.empty())
    {
        return ErrNoError;
    }

    auto baseHelper = std::make_shared<realDecoder>();
    baseHelper->m_raw = buf;
    baseHelper->m_metric_registry = metricRegistry;

    auto err = prepareFlexibleDecoder(baseHelper.get(), in, version);
    if (err != ErrNoError)
    {
        return err;
    }

    int remaining = baseHelper->remaining();
    if (remaining != 0)
    {
        throw PacketDecodingError{"invalid length len=" + std::to_string(buf.size()) + " remaining=" + std::to_string(remaining)};
    }

    return ErrNoError;
}

int prepareFlexibleDecoder(PDecoder *pd, std::shared_ptr<VDecoder> req, int16_t version)
{
    if (auto fv = std::dynamic_pointer_cast<flexible_version>(req))
    {
        if (fv->is_flexible_version(version))
        {
            return req->decode(*dynamic_cast<realFlexibleDecoder *>(pd), version);
        }
    }

    return req->decode(*pd, version);
}

int prepareFlexibleEncoder(PEncoder *pe, std::shared_ptr<IEncoder> req)
{
    if (auto fv = std::dynamic_pointer_cast<flexible_version>(req))
    {
        if (fv->is_flexible())
        {
            if (dynamic_cast<prepEncoder *>(pe))
            {
                return req->encode(*dynamic_cast<prepEncoder *>(pe));
            }
            else if (dynamic_cast<realEncoder *>(pe))
            {
                return req->encode(*dynamic_cast<realEncoder *>(pe));
            }
        }
    }
    return req->encode(*pe);
}
std::shared_ptr<PDecoder> downgradeFlexibleDecoder(std::shared_ptr<PDecoder> pd)
{
    if (auto f = std::dynamic_pointer_cast<realFlexibleDecoder>(pd); f)
    {
        return f;
    }
    return pd;
}

inline constexpr int magicOffset = 16;
int magicValue(PDecoder &pd, int8_t &magic)
{
    return pd.peekInt8(magicOffset, magic);
}