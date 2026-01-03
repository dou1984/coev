#include "version.h"
#include "api_versions.h"
#include "delete_groups_request.h"

void DeleteGroupsRequest::setVersion(int16_t v)
{
    Version = v;
}

int DeleteGroupsRequest::encode(PEncoder &pe)
{
    if (!pe.putStringArray(Groups))
    {
        return ErrEncodeError;
    }
    pe.putEmptyTaggedFieldArray();
    return true;
}

int DeleteGroupsRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;
    if (pd.getStringArray(Groups) != ErrNoError)
    {
        return ErrDecodeError;
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t DeleteGroupsRequest::key() const
{
    return apiKeyDeleteGroups;
}

int16_t DeleteGroupsRequest::version() const
{
    return Version;
}

int16_t DeleteGroupsRequest::headerVersion() const
{
    return Version >= 2 ? 2 : 1;
}

bool DeleteGroupsRequest::isFlexible() const
{
    return isFlexibleVersion(Version);
}

bool DeleteGroupsRequest::isFlexibleVersion(int16_t version)
{
    return version >= 2;
}

bool DeleteGroupsRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 2;
}

KafkaVersion DeleteGroupsRequest::requiredVersion() const
{
    switch (Version)
    {
    case 2:
        return V2_4_0_0;
    case 1:
        return V2_0_0_0;
    case 0:
        return V1_1_0_0;
    default:
        return V2_0_0_0;
    }
}

void DeleteGroupsRequest::AddGroup(const std::string &group)
{
    Groups.push_back(group);
}