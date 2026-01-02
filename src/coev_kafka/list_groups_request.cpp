#include "version.h"
#include "list_groups_request.h"
#include "api_versions.h"

void ListGroupsRequest::setVersion(int16_t v)
{
    Version = v;
}

int ListGroupsRequest::encode(PEncoder &pe)
{
    if (Version >= 4)
    {
        int err = pe.putArrayLength(static_cast<int32_t>(StatesFilter.size()));
        if (err != 0)
            return err;

        for (auto &filter : StatesFilter)
        {
            err = pe.putString(filter);
            if (err != 0)
                return err;
        }

        if (Version >= 5)
        {
            err = pe.putArrayLength(static_cast<int32_t>(TypesFilter.size()));
            if (err != 0)
                return err;

            for (auto &filter : TypesFilter)
            {
                err = pe.putString(filter);
                if (err != 0)
                    return err;
            }
        }
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int ListGroupsRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    if (Version >= 4)
    {
        int err = pd.getStringArray(StatesFilter);
        if (err != 0)
            return err;

        if (Version >= 5)
        {
            err = pd.getStringArray(TypesFilter);
            if (err != 0)
                return err;
        }
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t ListGroupsRequest::key() const
{
    return apiKeyListGroups;
}

int16_t ListGroupsRequest::version() const
{
    return Version;
}

int16_t ListGroupsRequest::headerVersion() const
{
    return (Version >= 3) ? 2 : 1;
}

bool ListGroupsRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 5;
}

bool ListGroupsRequest::isFlexible()
{
    return isFlexibleVersion(Version);
}

bool ListGroupsRequest::isFlexibleVersion(int16_t ver)
{
    return ver >= 3;
}

KafkaVersion ListGroupsRequest::requiredVersion() const
{
    switch (Version)
    {
    case 5:
        return V3_8_0_0;
    case 4:
        return V2_6_0_0;
    case 3:
        return V2_4_0_0;
    case 2:
        return V2_0_0_0;
    case 1:
        return V0_11_0_0;
    case 0:
        return V0_9_0_0;
    default:
        return V2_6_0_0;
    }
}