
#include <string>
#include "encoder_decoder.h"
#include "prep_encoder.h"
#include "real_encoder.h"
#include "request.h"
#include "errors.h"
#include "packet_decoding_error.h"

int encode(IEncoder &e, std::string &out)
{
    prep_encoder enc;
    if (prepare_flexible_encoder(enc, e) != ErrNoError)
    {
        throw PacketEncodingError{"encoding failed"};
    }

    if (enc.m_length < 0 || enc.m_length > static_cast<int>(MaxRequestSize))
    {
        throw PacketEncodingError{"invalid request size (" + std::to_string(enc.m_length) + ")"};
    }

    real_encoder real_enc;
    real_enc.m_raw.resize(enc.m_length);
    if (prepare_flexible_encoder(real_enc, e) != ErrNoError)
    {
        throw PacketEncodingError{"encoding failed"};
    }

    if (out.empty())
    {
        out = std::move(real_enc.m_raw);
    }
    else
    {
        out.append(real_enc.m_raw);
    }
    return ErrNoError;
}
int decode(const std::string &buf, IDecoder &in)
{
    if (buf.empty())
    {
        return ErrNoError;
    }

    real_decoder helper;
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

int versioned_decode(const std::string &buf, VDecoder &in, int16_t version)
{
    if (buf.empty())
    {
        return ErrNoError;
    }

    real_decoder baseHelper;
    baseHelper.m_raw = buf;

    auto err = prepare_flexible_decoder(baseHelper, in, version);
    if (err != ErrNoError)
    {
        return err;
    }

    int remaining = baseHelper.remaining();
    if (remaining != 0)
    {
        throw PacketDecodingError{"invalid length len=" + std::to_string(buf.size()) + " remaining=" + std::to_string(remaining)};
    }

    return ErrNoError;
}

int prepare_flexible_decoder(packet_decoder &pd, VDecoder &req, int16_t version)
{
    auto f = dynamic_cast<flexible_version *>(&req);
    if (f != nullptr)
    {
        if (f->is_flexible_version(version))
        {
            pd.pushFlexible();
            defer(pd.popFlexible());
            return req.decode(pd, version);
        }
    }
    return req.decode(pd, version);
}

int prepare_flexible_encoder(packet_encoder &pe, IEncoder &req)
{
    auto f = dynamic_cast<flexible_version *>(&req);
    if (f != nullptr)
    {
        if (f->is_flexible())
        {
            pe.pushFlexible();
            defer(pe.popFlexible());
            return req.encode(pe);
        }
    }
    return req.encode(pe);
}
std::shared_ptr<packet_decoder> downgrade_flexible_decoder(std::shared_ptr<packet_decoder> pd)
{
    if (pd->isFlexible())
    {
        pd->pushFlexible();
    }
    return pd;
}

inline constexpr int magic_offset = 16;
int magic_value(packet_decoder &pd, int8_t &magic)
{
    return pd.peekInt8(magic_offset, magic);
}
