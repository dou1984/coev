#include "version.h"
#include "list_groups_response.h"
#include <vector>
#include "api_versions.h"

void ListGroupsResponse::setVersion(int16_t v)
{
    Version = v;
}

int ListGroupsResponse::encode(PEncoder &pe)
{
    if (Version >= 1)
    {
        pe.putDurationMs(ThrottleTime);
    }

    pe.putKError(Err);

    int err = pe.putArrayLength(static_cast<int32_t>(Groups.size()));
    if (err != 0)
        return err;

    for (auto &kv : Groups)
    {
        const std::string &groupId = kv.first;
        const std::string &protocolType = kv.second;

        err = pe.putString(groupId);
        if (err != 0)
            return err;

        err = pe.putString(protocolType);
        if (err != 0)
            return err;

        if (Version >= 4)
        {
            const GroupData &gd = GroupsData[groupId]; // safe: assumed populated
            err = pe.putString(gd.GroupState);
            if (err != 0)
                return err;
        }

        if (Version >= 5)
        {
            const GroupData &gd = GroupsData[groupId];
            err = pe.putString(gd.GroupType);
            if (err != 0)
                return err;
        }

        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int ListGroupsResponse::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    if (Version >= 1)
    {
        int err = pd.getDurationMs(ThrottleTime);
        if (err != 0)
            return err;
    }

    int err = pd.getKError(Err);
    if (err != 0)
        return err;

    int32_t n;
    err = pd.getArrayLength(n);
    if (err != 0)
        return err;

    if (n > 0)
    {
        Groups.clear();
        Groups.reserve(n);
        if (Version >= 4)
        {
            GroupsData.clear();
            GroupsData.reserve(n);
        }
    }

    for (int32_t i = 0; i < n; ++i)
    {
        std::string groupId, protocolType;
        err = pd.getString(groupId);
        if (err != 0)
            return err;

        err = pd.getString(protocolType);
        if (err != 0)
            return err;

        Groups[groupId] = protocolType;

        if (Version >= 4)
        {
            GroupData gd;
            err = pd.getString(gd.GroupState);
            if (err != 0)
                return err;

            if (Version >= 5)
            {
                err = pd.getString(gd.GroupType);
                if (err != 0)
                    return err;
            }

            GroupsData[groupId] = std::move(gd);
        }
        int32_t _;
        err = pd.getEmptyTaggedFieldArray(_);
        if (err != 0)
            return err;
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t ListGroupsResponse::key() const
{
    return apiKeyListGroups;
}

int16_t ListGroupsResponse::version() const
{
    return Version;
}

int16_t ListGroupsResponse::headerVersion() const
{
    return (Version >= 3) ? 1 : 0;
}

bool ListGroupsResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 5;
}

bool ListGroupsResponse::isFlexible()
{
    return isFlexibleVersion(Version);
}

bool ListGroupsResponse::isFlexibleVersion(int16_t ver)
{
    return ver >= 3;
}

KafkaVersion ListGroupsResponse::requiredVersion() const
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