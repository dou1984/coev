#include <vector>
#include <stdexcept>
#include <tuple>
#include <iostream>
#include "request.h"
#include "broker.h"
#include "api_versions.h"
#include "protocol_body.h"
#include "length_field.h"
#include "packet_decoder.h"
#include "packet_encoder.h"
#include "request_decode.h"

int Request::encode(packet_encoder &pe) const
{

    LengthField length_field;
    pe.push(length_field);

    pe.putInt16(m_body->key());
    pe.putInt16(m_body->version());
    pe.putInt32(m_correlation_id);

    int err = 0;
    if (m_body->header_version() >= 1)
    {
        err = pe.putString(m_client_id);
        if (err != 0)
        {
            return err;
        }
    }

    if (m_body->header_version() >= 2)
    {
        pe.putUVarint(0);
    }

    bool is_flexible = false;
    auto f = dynamic_cast<const flexible_version *>(m_body);
    if (f != nullptr)
    {
        is_flexible = f->is_flexible_version(m_body->version());
    }

    if (is_flexible)
    {
        pe.__push_flexible();
        defer(pe.__pop_flexible());

        err = const_cast<protocol_body *>(m_body)->encode(pe);
        if (err != 0)
        {
            return err;
        }
        return pe.pop();
    }
    else
    {
        err = const_cast<protocol_body *>(m_body)->encode(pe);
        if (err != 0)
        {
            return err;
        }
        return pe.pop();
    }
}
int Request::encode(packet_encoder &pe, int16_t version) const
{
    return encode(pe);
}
int Request::decode(packet_decoder &pd)
{
    int16_t key, version;

    std::string clientID;
    int err;

    err = pd.getInt16(key);
    if (err != 0)
        return err;

    err = pd.getInt16(version);
    if (err != 0)
        return err;

    int32_t correlationID = 0;
    err = pd.getInt32(correlationID);
    if (err != 0)
        return err;
    m_correlation_id = correlationID;

    err = pd.getString(clientID);
    if (err != 0)
        return err;
    m_client_id = clientID;

    auto body = request_allocate(key, version);
    m_body = body.get();
    if (!m_body)
    {
        return -1;
    }

    if (m_body->header_version() >= 2)
    {
        uint64_t tagCount;
        err = pd.getUVariant(tagCount);
        if (err != 0)
            return err;
    }

    return prepare_flexible_decoder(pd, *const_cast<protocol_body *>(m_body), version);
}

bool Request::is_flexible() const
{

    if (m_body)
    {
        auto f = dynamic_cast<const flexible_version *>(m_body);
        if (f != nullptr)
        {
            return f->is_flexible();
        }
    }
    return false;
}