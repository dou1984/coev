
#include <string>
#include "encoder_decoder.h"
#include "prep_encoder.h"
#include "real_encoder.h"
#include "request.h"
#include "errors.h"
#include "packet_error.h"

inline constexpr int magic_offset = 16;
int encode(IEncoder &e, std::string &out)
{
    prep_encoder enc;
    if (prepare_flexible_encoder(enc, e) != ErrNoError)
    {
        throw PacketError{"encoding failed"};
    }

    if (enc.m_length < 0 || enc.m_length > static_cast<int>(MaxRequestSize))
    {
        throw PacketError{"invalid request size (" + std::to_string(enc.m_length) + ")"};
    }

    real_encoder real_enc;
    real_enc.m_raw.resize(enc.m_length);
    if (prepare_flexible_encoder(real_enc, e) != ErrNoError)
    {
        throw PacketError{"encoding failed"};
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

    real_decoder helper(buf);
    if (in.decode(helper) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (helper.m_offset != static_cast<int>(buf.size()))
    {
        throw PacketError{"invalid length: buf=" + std::to_string(buf.size()) + " decoded=" + std::to_string(helper.m_offset)};
    }

    return ErrNoError;
}

int decode_version(const std::string &buf, VDecoder &in, int16_t version)
{
    if (buf.empty())
    {
        return ErrNoError;
    }

    real_decoder helper(buf);

    auto err = prepare_flexible_decoder(helper, in, version);
    if (err != ErrNoError)
    {
        return err;
    }

    int remaining = helper.remaining();
    if (remaining != 0)
    {
        throw PacketError{"invalid length len=" + std::to_string(buf.size()) + " remaining=" + std::to_string(remaining)};
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
            pd.__push_flexible();
            defer(pd.__pop_flexible());
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
            pe.__push_flexible();
            defer(pe.__pop_flexible());
            return req.encode(pe);
        }
    }
    return req.encode(pe);
}
int magic_value(packet_decoder &pd, int8_t &magic)
{
    return pd.peekInt8(magic_offset, magic);
}

// std::shared_ptr<packet_decoder> downgrade_flexible_decoder(std::shared_ptr<packet_decoder> pd)
// {
//     if (pd->__is_flexible())
//     {
//         pd->__push_flexible();
//     }
//     return pd;
// }
