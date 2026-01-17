
#include <string>
#include "encoder_decoder.h"
#include "prep_encoder.h"
#include "real_encoder.h"
#include "request.h"
#include "errors.h"
#include "packet_decoding_error.h"

int encode(IEncoder &e, std::string &out)
{
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
    if (prepareFlexibleEncoder(realEnc.get(), e) != ErrNoError)
    {
        throw PacketEncodingError{"encoding failed"};
    }

    if (out.empty())
    {
        out = std::move(realEnc->m_raw);
    }
    else
    {
        out.append(realEnc->m_raw);
    }
    return ErrNoError;
}
int decode(const std::string &buf, IDecoder &in)
{
    if (buf.empty())
    {
        return ErrNoError;
    }

    realDecoder helper;
    helper.m_raw = buf;

    if (in.decode(helper) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (helper.m_offset != static_cast<int>(buf.size()))
    {
        throw PacketDecodingError{"invalid length: buf=" + std::to_string(buf.size()) + " decoded=" + std::to_string(helper.m_offset)};
    }

    return ErrNoError;
}

int versionedDecode(const std::string &buf, versioned_decoder &in, int16_t version)
{
    if (buf.empty())
    {
        return ErrNoError;
    }

    auto baseHelper = std::make_shared<realDecoder>();
    baseHelper->m_raw = buf;

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

int prepareFlexibleDecoder(packetDecoder *pd, versioned_decoder &req, int16_t version)
{
    if (auto fv = dynamic_cast<flexible_version *>(&req))
    {
        if (fv->is_flexible_version(version))
        {
            if (auto realDec = dynamic_cast<realDecoder *>(pd))
            {
                realFlexibleDecoder flexibleDec;
                flexibleDec.m_raw = realDec->m_raw;
                flexibleDec.m_offset = realDec->m_offset;
                flexibleDec.m_stack = realDec->m_stack;

                int err = req.decode(flexibleDec, version);

                realDec->m_offset = flexibleDec.m_offset;
                realDec->m_stack = flexibleDec.m_stack;

                return err;
            }
        }
    }
    return req.decode(*pd, version);
}

int prepareFlexibleEncoder(packetEncoder *pe, IEncoder &req)
{
    bool is_flexible = false;

    // Check if the request is directly a flexible version
    if (auto fv = dynamic_cast<flexible_version *>(&req))
    {
        is_flexible = fv->is_flexible();
    }
    // Check if the request is a request object with a flexible body
    else if (auto request_obj = dynamic_cast<request *>(&req))
    {
        is_flexible = request_obj->is_flexible();
    }

    if (is_flexible)
    {
        if (auto prepEnc = dynamic_cast<prepEncoder *>(pe))
        {
            // Create a flexible prep encoder wrapper
            prepFlexibleEncoder flexiblePrepEnc(*prepEnc);
            // Call encode on the flexible wrapper
            return req.encode(flexiblePrepEnc);
        }
        else if (auto realEnc = dynamic_cast<realEncoder *>(pe))
        {
            // Create a flexible real encoder wrapper
            realFlexibleEncoder flexibleRealEnc(*realEnc);
            // Call encode on the flexible wrapper
            return req.encode(flexibleRealEnc);
        }
    }
    // Not flexible, call encode on the original encoder
    return req.encode(*pe);
}
std::shared_ptr<packetDecoder> downgradeFlexibleDecoder(std::shared_ptr<packetDecoder> pd)
{
    if (auto f = std::dynamic_pointer_cast<realFlexibleDecoder>(pd); f)
    {
        return f;
    }
    return pd;
}

inline constexpr int magicOffset = 16;
int magicValue(packetDecoder &pd, int8_t &magic)
{
    return pd.peekInt8(magicOffset, magic);
}