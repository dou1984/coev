#include "version.h"
#include "describe_groups_request.h"

void DescribeGroupsRequest::setVersion(int16_t v)
{
    Version = v;
}

int DescribeGroupsRequest::encode(PEncoder &pe)
{
    if (!pe.putStringArray(Groups))
    {
        return ErrEncodeError;
    }
    if (Version >= 3)
    {
        pe.putBool(IncludeAuthorizedOperations);
    }
    pe.putEmptyTaggedFieldArray();
    return true;
}

int DescribeGroupsRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;
    if (!pd.getStringArray(Groups))
    {
        return ErrDecodeError;
    }
    if (Version >= 3)
    {
        bool val;
        if (!pd.getBool(val))
        {
            return ErrDecodeError;
        }
        IncludeAuthorizedOperations = val;
    }
    int32_t dummy;
    if (!pd.getEmptyTaggedFieldArray(dummy))
    {
        return ErrDecodeError;
    }
    return ErrNoError;
}

int16_t DescribeGroupsRequest::key() const
{
    return apiKeyDescribeGroups;
}

int16_t DescribeGroupsRequest::version() const
{
    return Version;
}

int16_t DescribeGroupsRequest::headerVersion() const
{
    if (Version >= 5)
    {
        return 2;
    }
    return 1;
}

bool DescribeGroupsRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 5;
}

bool DescribeGroupsRequest::isFlexible() const
{
    return isFlexibleVersion(Version);
}

bool DescribeGroupsRequest::isFlexibleVersion(int16_t version)
{
    return version >= 5;
}

KafkaVersion DescribeGroupsRequest::requiredVersion() const
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

void DescribeGroupsRequest::AddGroup(const std::string &group)
{
    Groups.push_back(group);
}