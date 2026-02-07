#include "version.h"
#include "join_group_request.h"
#include "api_versions.h"

GroupProtocol::GroupProtocol(const std::string &name, const std::string &metadata) : m_name(name), m_metadata(metadata)
{
}
int GroupProtocol::decode(packet_decoder &pd)
{
    int err = pd.getString(m_name);
    if (err != 0)
        return err;

    err = pd.getBytes(m_metadata);
    if (err != 0)
        return err;
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int GroupProtocol::encode(packet_encoder &pe) const
{
    int err = pe.putString(m_name);
    if (err != 0)
        return err;

    err = pe.putBytes(m_metadata);
    if (err != 0)
        return err;

    pe.putEmptyTaggedFieldArray();
    return 0;
}

void JoinGroupRequest::set_version(int16_t v)
{
    m_version = v;
}

int JoinGroupRequest::encode(packet_encoder &pe) const
{
    int err = pe.putString(m_group_id);
    if (err != 0)
        return err;

    pe.putInt32(m_session_timeout);

    if (m_version >= 1)
    {
        pe.putInt32(m_rebalance_timeout);
    }

    err = pe.putString(m_member_id);
    if (err != 0)
        return err;

    if (m_version >= 5)
    {

        err = pe.putNullableString(m_group_instance_id);

        if (err != 0)
            return err;
    }

    err = pe.putString(m_protocol_type);
    if (err != 0)
        return err;

    if (!m_group_protocols.empty())
    {
        if (!m_ordered_group_protocols.empty())
        {
            return -1;
        }

        err = pe.putArrayLength(static_cast<int32_t>(m_group_protocols.size()));
        if (err != 0)
            return err;

        for (auto &kv : m_group_protocols)
        {
            err = pe.putString(kv.first);
            if (err != 0)
                return err;
            err = pe.putBytes(kv.second);
            if (err != 0)
                return err;
            pe.putEmptyTaggedFieldArray();
        }
    }
    else
    {
        err = pe.putArrayLength(static_cast<int32_t>(m_ordered_group_protocols.size()));
        if (err != 0)
            return err;

        for (auto &protocol : m_ordered_group_protocols)
        {
            err = protocol.encode(pe);
            if (err != 0)
                return err;
        }
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int JoinGroupRequest::decode(packet_decoder &pd, int16_t version)
{
    m_version = version;

    int err = pd.getString(m_group_id);
    if (err != 0)
        return err;

    err = pd.getInt32(m_session_timeout);
    if (err != 0)
        return err;

    if (version >= 1)
    {
        err = pd.getInt32(m_rebalance_timeout);
        if (err != 0)
            return err;
    }

    err = pd.getString(m_member_id);
    if (err != 0)
        return err;

    if (version >= 5)
    {

        err = pd.getNullableString(m_group_instance_id);
        if (err != 0)
            return err;
    }

    err = pd.getString(m_protocol_type);
    if (err != 0)
        return err;

    int32_t n;
    err = pd.getArrayLength(n);
    if (err != 0)
        return err;

    if (n == 0)
    {
        return 0;
    }

    m_group_protocols.clear();
    m_ordered_group_protocols.clear();

    for (int32_t i = 0; i < n; ++i)
    {
        GroupProtocol protocol;
        err = protocol.decode(pd);
        if (err != 0)
        {
            return err;
        }

        m_group_protocols[protocol.m_name] = protocol.m_metadata;
        m_ordered_group_protocols.emplace_back(protocol.m_name, protocol.m_metadata);
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t JoinGroupRequest::key() const
{
    return apiKeyJoinGroup;
}

int16_t JoinGroupRequest::version() const
{
    return m_version;
}

int16_t JoinGroupRequest::header_version() const
{
    return (m_version >= 6) ? 2 : 1;
}

bool JoinGroupRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 6;
}

bool JoinGroupRequest::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool JoinGroupRequest::is_flexible_version(int16_t ver) const
{
    return ver >= 6;
}

KafkaVersion JoinGroupRequest::required_version() const
{
    switch (m_version)
    {
    case 6:
        return V2_4_0_0;
    case 5:
        return V2_3_0_0;
    case 4:
        return V2_2_0_0;
    case 3:
        return V2_0_0_0;
    case 2:
        return V0_11_0_0;
    case 1:
        return V0_10_1_0;
    case 0:
        return V0_10_0_0;
    default:
        return V2_3_0_0;
    }
}

void JoinGroupRequest::add_group_protocol(const std::string &name, const std::string &metadata)
{
    m_ordered_group_protocols.emplace_back(name, metadata);
}

int JoinGroupRequest::add_group_protocol_metadata(const std::string &name, const std::shared_ptr<ConsumerGroupMemberMetadata> &metadata)
{
    return -1;
}