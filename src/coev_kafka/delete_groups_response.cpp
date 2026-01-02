#include "version.h"
#include "api_versions.h"
#include "delete_groups_response.h"

void DeleteGroupsResponse::setVersion(int16_t v)
{
    Version = v;
}

int DeleteGroupsResponse::encode(PEncoder &pe)
{
    pe.putDurationMs(ThrottleTime);

    if (!pe.putArrayLength(static_cast<int32_t>(GroupErrorCodes.size())))
    {
        return ErrEncodeError;
    }

    for (auto &kv : GroupErrorCodes)
    {
        if (!pe.putString(kv.first))
        {
            return ErrEncodeError;
        }
        pe.putInt16(static_cast<int16_t>(kv.second));
        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return true;
}

int DeleteGroupsResponse::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    if (!pd.getDurationMs(ThrottleTime))
    {
        return ErrDecodeError;
    }

    int32_t n;
    if (!pd.getArrayLength(n))
    {
        return ErrDecodeError;
    }

    GroupErrorCodes.clear();

    if (n > 0)
    {
        for (int32_t i = 0; i < n; ++i)
        {
            std::string groupID;
            if (!pd.getString(groupID))
            {
                return ErrDecodeError;
            }

            int16_t errCode;
            if (!pd.getInt16(errCode))
            {
                return ErrDecodeError;
            }
            GroupErrorCodes[groupID] = static_cast<KError>(errCode);
            int32_t _;
            if (!pd.getEmptyTaggedFieldArray(_))
            {
                return ErrDecodeError;
            }
        }
    }

    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t DeleteGroupsResponse::key() const
{
    return apiKeyDeleteGroups;
}

int16_t DeleteGroupsResponse::version() const
{
    return Version;
}

int16_t DeleteGroupsResponse::headerVersion() const
{
    return Version >= 2 ? 1 : 0;
}

bool DeleteGroupsResponse::isFlexible() const
{
    return isFlexibleVersion(Version);
}

bool DeleteGroupsResponse::isFlexibleVersion(int16_t version)
{
    return version >= 2;
}

bool DeleteGroupsResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 2;
}

KafkaVersion DeleteGroupsResponse::requiredVersion() const
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

std::chrono::milliseconds DeleteGroupsResponse::throttleTime() const
{
    return ThrottleTime;
}