/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */

#include <string>
#include "encoder_decoder.h"
#include "prep_encoder.h"
#include "real_encoder.h"
#include "request.h"
#include "errors.h"
#include "packet_error.h"

namespace coev::kafka
{
    inline constexpr int magic_offset = 16;
    int encode(const IEncoder &e, std::string &out)
    {
        PacketEncoder enc(PacketEncoder::PREP);
        if (prepare_flexible_encoder(enc, e) != ErrNoError)
        {
            LOG_ERR("encode PREP failed");
            return ErrEncodeError;
        }

        if (enc.m_length < 0 || enc.m_length > static_cast<int>(MaxRequestSize))
        {
            return ErrEncodeError;
        }

        PacketEncoder real_enc(PacketEncoder::REAL);
        real_enc.m_raw.resize(enc.m_length);
        if (prepare_flexible_encoder(real_enc, e) != ErrNoError)
        {
            return ErrEncodeError;
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

        PacketDecoder helper(buf);
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

    int decode_version(std::string_view buf, VDecoder &in, int16_t version)
    {
        if (buf.empty())
        {
            return ErrNoError;
        }

        PacketDecoder helper(buf);

        auto err = prepare_flexible_decoder(helper, in, version);
        if (err != ErrNoError)
        {
            return err;
        }

        int remaining = helper.remaining();
        if (remaining != 0)
        {
            LOG_DBG("decode_version remaining:%d offset:%d size:%ld version:%d", remaining, helper.m_offset, buf.size(), version);
            return ErrDecodeError;
        }

        LOG_CORE("decode_version done remaining:%d offset:%d size:%ld version:%d", remaining, helper.m_offset, buf.size(), version);
        return ErrNoError;
    }

    int prepare_flexible_decoder(PacketDecoder &pd, VDecoder &req, int16_t version)
    {
        auto body = dynamic_cast<protocol_body *>(&req);
        if (body != nullptr && body->is_flexible_version(version))
        {
            pd.__push_flexible();
            finally(pd.__pop_flexible());
            int err = req.decode(pd, version);
            return err;
        }
        return req.decode(pd, version);
    }

    int prepare_flexible_encoder(PacketEncoder &pe, const IEncoder &req)
    {
        auto body = dynamic_cast<const protocol_body *>(&req);
        if (body != nullptr && body->is_flexible())
        {
            pe.__push_flexible();
            finally(pe.__pop_flexible());
            return req.encode(pe);
        }
        return req.encode(pe);
    }
    int magic_value(PacketDecoder &pd, int8_t &magic)
    {
        return pd.peekInt8(magic_offset, magic);
    }
}