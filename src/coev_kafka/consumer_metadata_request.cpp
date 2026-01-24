#include "consumer_metadata_request.h"
#include "find_coordinator_request.h"

ConsumerMetadataRequest::ConsumerMetadataRequest() : m_version(0)
{
}
ConsumerMetadataRequest::ConsumerMetadataRequest(int16_t v, const std::string &group) : m_version(v), m_consumer_group(group)
{
}

void ConsumerMetadataRequest::set_version(int16_t v)
{
    m_version = v;
}

int ConsumerMetadataRequest::encode(packetEncoder &pe)
{
    FindCoordinatorRequest tmp;
    tmp.m_coordinator_key = m_consumer_group;
    tmp.m_coordinator_type = CoordinatorGroup;
    tmp.m_version = m_version;
    return tmp.encode(pe);
}

int ConsumerMetadataRequest::decode(packetDecoder &pd, int16_t version)
{
    FindCoordinatorRequest tmp;
    int err = tmp.decode(pd, version);
    if (err != 0)
    {
        return err;
    }
    m_consumer_group = tmp.m_coordinator_key;
    return 0;
}

int16_t ConsumerMetadataRequest::key() const
{
    return apiKeyFindCoordinator;
}

int16_t ConsumerMetadataRequest::version() const
{
    return m_version;
}

int16_t ConsumerMetadataRequest::header_version() const
{
    return 1;
}

bool ConsumerMetadataRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 2;
}

KafkaVersion ConsumerMetadataRequest::required_version() const
{
    return V0_9_0_0;
}