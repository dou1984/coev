#include "version.h"
#include "join_group_request.h"
#include "api_versions.h"
int GroupProtocol::decode(PDecoder &pd)
{
    int err = pd.getString(Name);
    if (err != 0)
        return err;

    err = pd.getBytes(Metadata);
    if (err != 0)
        return err;
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int GroupProtocol::encode(PEncoder &pe)
{
    int err = pe.putString(Name);
    if (err != 0)
        return err;

    err = pe.putBytes(Metadata);
    if (err != 0)
        return err;

    pe.putEmptyTaggedFieldArray();
    return 0;
}

void JoinGroupRequest::setVersion(int16_t v)
{
    Version = v;
}

int JoinGroupRequest::encode(PEncoder &pe)
{
    int err = pe.putString(GroupId);
    if (err != 0)
        return err;

    pe.putInt32(SessionTimeout);

    if (Version >= 1)
    {
        pe.putInt32(RebalanceTimeout);
    }

    err = pe.putString(MemberId);
    if (err != 0)
        return err;

    if (Version >= 5)
    {

        err = pe.putNullableString(GroupInstanceId);

        if (err != 0)
            return err;
    }

    err = pe.putString(ProtocolType);
    if (err != 0)
        return err;

    if (!GroupProtocols.empty())
    {
        if (!OrderedGroupProtocols.empty())
        {
            return -1;
        }

        err = pe.putArrayLength(static_cast<int32_t>(GroupProtocols.size()));
        if (err != 0)
            return err;

        for (auto &kv : GroupProtocols)
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
        err = pe.putArrayLength(static_cast<int32_t>(OrderedGroupProtocols.size()));
        if (err != 0)
            return err;

        for (auto &protocol : OrderedGroupProtocols)
        {
            err = protocol->encode(pe);
            if (err != 0)
                return err;
        }
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int JoinGroupRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    int err = pd.getString(GroupId);
    if (err != 0)
        return err;

    err = pd.getInt32(SessionTimeout);
    if (err != 0)
        return err;

    if (version >= 1)
    {
        err = pd.getInt32(RebalanceTimeout);
        if (err != 0)
            return err;
    }

    err = pd.getString(MemberId);
    if (err != 0)
        return err;

    if (version >= 5)
    {

        err = pd.getNullableString(GroupInstanceId);
        if (err != 0)
            return err;
    }

    err = pd.getString(ProtocolType);
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

    GroupProtocols.clear();
    OrderedGroupProtocols.clear();

    for (int32_t i = 0; i < n; ++i)
    {
        auto protocol = std::make_shared<GroupProtocol>();
        err = protocol->decode(pd);
        if (err != 0)
            return err;

        GroupProtocols[protocol->Name] = protocol->Metadata;
        OrderedGroupProtocols.push_back(protocol);
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
    return Version;
}

int16_t JoinGroupRequest::headerVersion() const
{
    return (Version >= 6) ? 2 : 1;
}

bool JoinGroupRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 6;
}

bool JoinGroupRequest::isFlexible()
{
    return isFlexibleVersion(Version);
}

bool JoinGroupRequest::isFlexibleVersion(int16_t ver)
{
    return ver >= 6;
}

KafkaVersion JoinGroupRequest::requiredVersion() const
{
    switch (Version)
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

void JoinGroupRequest::AddGroupProtocol(const std::string &name, const std::string &metadata)
{
    auto protocol = std::make_shared<GroupProtocol>();
    protocol->Name = name;
    protocol->Metadata = metadata;
    OrderedGroupProtocols.push_back(protocol);
}

int JoinGroupRequest::AddGroupProtocolMetadata(const std::string &name, const std::shared_ptr< ConsumerGroupMemberMetadata >&metadata)
{
    return -1;
}