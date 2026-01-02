#include "version.h"
#include "find_coordinator_request.h"
#include "api_versions.h"

FindCoordinatorRequest::FindCoordinatorRequest()
    : Version(0), CoordinatorType_(CoordinatorGroup) {}

void FindCoordinatorRequest::setVersion(int16_t v)
{
    Version = v;
}

int FindCoordinatorRequest::encode(PEncoder &pe)
{
    int err = pe.putString(CoordinatorKey);
    if (err != 0)
        return err;

    if (Version >= 1)
    {
        pe.putInt8(static_cast<int8_t>(CoordinatorType_));
    }

    return 0;
}

int FindCoordinatorRequest::decode(PDecoder &pd, int16_t version)
{
    int err = pd.getString(CoordinatorKey);
    if (err != 0)
        return err;

    if (version >= 1)
    {
        Version = version;
        int8_t coordinatorType;
        err = pd.getInt8(coordinatorType);
        if (err != 0)
            return err;
        CoordinatorType_ = static_cast<CoordinatorType>(coordinatorType);
    }

    return 0;
}

int16_t FindCoordinatorRequest::key() const
{
    return apiKeyFindCoordinator;
}

int16_t FindCoordinatorRequest::version() const
{
    return Version;
}

int16_t FindCoordinatorRequest::headerVersion() const
{
    return 1;
}

bool FindCoordinatorRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 2;
}

KafkaVersion FindCoordinatorRequest::requiredVersion() const
{
    switch (Version)
    {
    case 2:
        return V2_0_0_0;
    case 1:
        return V0_11_0_0;
    default:
        return V0_8_2_0;
    }
}