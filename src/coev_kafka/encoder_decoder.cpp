
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
    if (!prepareFlexibleEncoder(prepEnc.get(), e))
    {
        throw PacketEncodingError{"encoding failed"};
    }

    if (prepEnc->Length < 0 || prepEnc->Length > static_cast<int>(MaxRequestSize))
    {
        throw PacketEncodingError{"invalid request size (" + std::to_string(prepEnc->Length) + ")"};
    }

    auto realEnc = std::make_shared<realEncoder>();
    realEnc->Raw.resize(prepEnc->Length);
    realEnc->Registry = metricRegistry;
    if (!prepareFlexibleEncoder(realEnc.get(), e))
    {
        throw PacketEncodingError{"encoding failed"};
    }

    out.insert(out.end(), realEnc->Raw.begin(), realEnc->Raw.end());
    return ErrNoError;
}
int decode(const std::string &buf, std::shared_ptr<IDecoder> in, std::shared_ptr<metrics::Registry> metricRegistry)
{
    if (buf.empty())
    {
        return ErrNoError;
    }

    realDecoder helper;
    helper.Raw = buf;
    helper.MetricRegistry = metricRegistry;

    if (!in->decode(helper))
    {
        return ErrDecodeError;
    }

    if (helper.Off != static_cast<int>(buf.size()))
    {
        throw PacketDecodingError{"invalid length: buf=" + std::to_string(buf.size()) + " decoded=" + std::to_string(helper.Off)};
    }

    return ErrNoError;
}

bool versionedDecode(const std::string &buf, const std::shared_ptr<VDecoder> &in, int16_t version, std::shared_ptr<metrics::Registry> &metricRegistry)
{
    if (buf.empty())
    {
        return true;
    }

    auto baseHelper = std::make_shared<realDecoder>();
    baseHelper->Raw = buf;
    baseHelper->MetricRegistry = metricRegistry;

    auto err = prepareFlexibleDecoder(baseHelper.get(), in, version);
    if (err)
    {
        return false;
    }

    int remaining = baseHelper->remaining();
    if (remaining != 0)
    {
        throw PacketDecodingError{"invalid length len=" + std::to_string(buf.size()) + " remaining=" + std::to_string(remaining)};
    }

    return true;
}

int prepareFlexibleDecoder(PDecoder *pd, std::shared_ptr<VDecoder> req, int16_t version)
{
    if (auto fv = std::dynamic_pointer_cast<flexibleVersion>(req))
    {
        if (fv->isFlexibleVersion(version))
        {
            return req->decode(*dynamic_cast<realFlexibleDecoder *>(pd), version);
        }
    }

    return req->decode(*pd, version);
}

int prepareFlexibleEncoder(PEncoder *pe, std::shared_ptr<IEncoder> req)
{
    if (auto fv = std::dynamic_pointer_cast<flexibleVersion>(req))
    {
        if (fv->isFlexible())
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