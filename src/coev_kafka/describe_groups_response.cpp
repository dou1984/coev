#include "describe_groups_response.h"

void DescribeGroupsResponse::setVersion(int16_t v)
{
    Version = v;
}

int DescribeGroupsResponse::encode(PEncoder &pe)
{
    if (Version >= 1)
    {
        pe.putDurationMs(ThrottleTime);
    }
    if (!pe.putArrayLength(static_cast<int32_t>(Groups.size())))
    {
        return ErrEncodeError;
    }

    for (auto &block : Groups)
    {
        if (!block->encode(pe, Version))
        {
            return ErrEncodeError;
        }
    }

    pe.putEmptyTaggedFieldArray();
    return true;
}

int DescribeGroupsResponse::decode(PDecoder &pd, int16_t version)
{
    Version = version;
    if (Version >= 1)
    {
        if (!pd.getDurationMs(ThrottleTime))
        {
            return ErrEncodeError;
        }
    }

    int32_t numGroups;
    if (!pd.getArrayLength(numGroups))
    {
        return ErrEncodeError;
    }

    Groups.clear();
    if (numGroups > 0)
    {
        Groups.resize(numGroups);
        for (int32_t i = 0; i < numGroups; ++i)
        {
            Groups[i] = std::make_shared<GroupDescription>();
            if (!Groups[i]->decode(pd, Version))
            {
                return ErrEncodeError;
            }
        }
    }

    int32_t dummy;
    if (!pd.getEmptyTaggedFieldArray(dummy))
    {
        return ErrEncodeError;
    }
    return true;
}

int16_t DescribeGroupsResponse::key() const
{
    return apiKeyDescribeGroups;
}

int16_t DescribeGroupsResponse::version() const
{
    return Version;
}

int16_t DescribeGroupsResponse::headerVersion() const
{
    if (Version >= 5)
    {
        return 1;
    }
    return 0;
}

bool DescribeGroupsResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 5;
}

bool DescribeGroupsResponse::isFlexible() const
{
    return isFlexibleVersion(Version);
}

bool DescribeGroupsResponse::isFlexibleVersion(int16_t version)
{
    return version >= 5;
}

KafkaVersion DescribeGroupsResponse::requiredVersion() const
{
    switch (Version)
    {
    case 5:
        return V2_4_0_0;
    case 4:
        return V2_4_0_0;
    case 3:
        return V2_3_0_0;
    case 2:
        return V2_0_0_0;
    case 1:
        return V0_11_0_0;
    case 0:
        return V0_9_0_0;
    default:
        return V2_4_0_0;
    }
}

std::chrono::milliseconds DescribeGroupsResponse::throttleTime() const
{
    return ThrottleTime;
}

int GroupDescription::encode(PEncoder &pe, int16_t version)
{
    Version = version;
    pe.putInt16(ErrorCode);

    if (!pe.putString(GroupId))
        return ErrEncodeError;
    if (!pe.putString(State))
        return ErrEncodeError;
    if (!pe.putString(ProtocolType))
        return ErrEncodeError;
    if (!pe.putString(Protocol))
        return ErrEncodeError;

    if (!pe.putArrayLength(static_cast<int32_t>(Members.size())))
    {
        return ErrEncodeError;
    }

    for (auto &pair : Members)
    {
        if (!pair.second->encode(pe, Version))
        {
            return ErrEncodeError;
        }
    }

    if (Version >= 3)
    {
        pe.putInt32(AuthorizedOperations);
    }

    pe.putEmptyTaggedFieldArray();
    return true;
}

int GroupDescription::decode(PDecoder &pd, int16_t version)
{
    Version = version;
    if (!pd.getInt16(ErrorCode))
        return ErrDecodeError;
    Err = static_cast<KError>(ErrorCode);

    if (!pd.getString(GroupId))
        return ErrDecodeError;
    if (!pd.getString(State))
        return ErrDecodeError;
    if (!pd.getString(ProtocolType))
        return ErrDecodeError;
    if (!pd.getString(Protocol))
        return ErrEncodeError;

    int32_t numMembers;
    if (!pd.getArrayLength(numMembers))
        return ErrDecodeError;

    Members.clear();
    if (numMembers > 0)
    {
        for (int32_t i = 0; i < numMembers; ++i)
        {
            auto block = std::make_shared<GroupMemberDescription>();
            if (!block->decode(pd, Version))
            {
                return ErrDecodeError;
            }
            Members[block->MemberId] = block;
        }
    }

    if (Version >= 3)
    {
        if (!pd.getInt32(AuthorizedOperations))
        {
            return ErrDecodeError;
        }
    }

    int32_t dummy;
    if (!pd.getEmptyTaggedFieldArray(dummy))
    {
        return ErrDecodeError;
    }
    return ErrNoError;
}

int GroupMemberDescription::encode(PEncoder &pe, int16_t version)
{
    Version = version;
    if (!pe.putString(MemberId))
        return ErrEncodeError;

    if (version >= 4)
    {
        if (!pe.putNullableString(GroupInstanceId))
            return ErrEncodeError;
    }

    if (!pe.putString(ClientId))
        return ErrEncodeError;
    if (!pe.putString(ClientHost))
        return ErrEncodeError;
    if (!pe.putBytes(MemberMetadata))
        return ErrEncodeError;
    if (!pe.putBytes(MemberAssignment))
        return ErrEncodeError;

    pe.putEmptyTaggedFieldArray();
    return true;
}

int GroupMemberDescription::decode(PDecoder &pd, int16_t version)
{
    Version = version;
    if (!pd.getString(MemberId))
        return ErrDecodeError;

    if (version >= 4)
    {
        if (!pd.getNullableString(GroupInstanceId))
            return ErrDecodeError;
    }

    if (!pd.getString(ClientId))
        return ErrDecodeError;
    if (!pd.getString(ClientHost))
        return ErrDecodeError;
    if (!pd.getBytes(MemberMetadata))
        return ErrDecodeError;
    if (!pd.getBytes(MemberAssignment))
        return ErrDecodeError;

    int32_t dummy;
    if (!pd.getEmptyTaggedFieldArray(dummy))
    {
        return ErrDecodeError;
    }
    return ErrNoError;
}

std::shared_ptr<ConsumerGroupMemberAssignment> GroupMemberDescription::GetMemberAssignment()
{
    if (MemberAssignment.empty())
    {
        return nullptr;
    }
    auto assignment = std::make_shared<ConsumerGroupMemberAssignment>();
    ::decode(MemberAssignment, std::dynamic_pointer_cast<IDecoder>(assignment), nullptr);
    return assignment;
}

std::shared_ptr<ConsumerGroupMemberMetadata> GroupMemberDescription::GetMemberMetadata()
{
    if (MemberMetadata.empty())
    {
        return nullptr;
    }
    auto metadata = std::make_shared<ConsumerGroupMemberMetadata>();
    ::decode(MemberMetadata, std::dynamic_pointer_cast<IDecoder>(metadata), nullptr);
    return metadata;
}