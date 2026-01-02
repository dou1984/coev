#include "consumer_metadata_request.h"
#include "find_coordinator_request.h"
void ConsumerMetadataRequest::set_version(int16_t v)
{
    Version = v;
}

int ConsumerMetadataRequest::encode(PEncoder &pe)
{
    FindCoordinatorRequest tmp;
    tmp.CoordinatorKey = ConsumerGroup;
    tmp.CoordinatorType_ = CoordinatorGroup;
    tmp.Version = Version;
    return tmp.encode(pe);
}

int ConsumerMetadataRequest::decode(PDecoder &pd, int16_t version)
{
    FindCoordinatorRequest tmp;
    int err = tmp.decode(pd, version);
    if (err != 0)
    {
        return err;
    }
    ConsumerGroup = tmp.CoordinatorKey;
    return 0;
}

int16_t ConsumerMetadataRequest::key() const
{
    return apiKeyFindCoordinator;
}

int16_t ConsumerMetadataRequest::version() const
{
    return Version;
}

int16_t ConsumerMetadataRequest::header_version() const
{
    return 1;
}

bool ConsumerMetadataRequest::is_valid_version() const
{
    return Version >= 0 && Version <= 2;
}