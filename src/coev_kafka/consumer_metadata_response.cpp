#include "consumer_metadata_response.h"
#include "find_coordinator_response.h"
#include "broker.h"
#include "version.h"
#include "api_versions.h"
#include <string>
#include <sstream>
#include <stdexcept>

ConsumerMetadataResponse::ConsumerMetadataResponse(int16_t v) : m_version(v)
{
}
void ConsumerMetadataResponse::set_version(int16_t v)
{
    m_version = v;
}

int ConsumerMetadataResponse::decode(packet_decoder &pd, int16_t version)
{
    auto response = std::make_shared<FindCoordinatorResponse>();

    if (int err = response->decode(pd, version); err != 0)
    {
        return err;
    }

    m_err = response->m_err;
    m_coordinator = response->m_coordinator;

    if (m_coordinator == nullptr)
    {
        return 0;
    }

    std::string addr = m_coordinator->Addr();
    size_t colonPos = addr.find_last_of(':');
    if (colonPos == std::string::npos)
    {
        return -1;
    }
    std::string host = addr.substr(0, colonPos);
    std::string portstr = addr.substr(colonPos + 1);

    try
    {
        int64_t port = std::stoll(portstr);
        m_coordinator_id = m_coordinator->ID();
        m_coordinator_host = host;
        m_coordinator_port = static_cast<int32_t>(port);
    }
    catch (...)
    {
        return -1;
    }

    return 0;
}

int ConsumerMetadataResponse::encode(packet_encoder &pe) const
{
    if (m_coordinator == nullptr)
    {
        m_coordinator = std::make_shared<Broker>();
        m_coordinator->m_id = m_coordinator_id;
        std::ostringstream oss;
        oss << m_coordinator_host << ":" << m_coordinator_port;
        m_coordinator->m_addr = oss.str();
    }

    FindCoordinatorResponse tmp;
    tmp.m_version = m_version;
    tmp.m_err = m_err;
    tmp.m_coordinator = m_coordinator;

    if (int err = tmp.encode(pe); err != 0)
    {
        return err;
    }

    return 0;
}

int16_t ConsumerMetadataResponse::key() const
{

    return apiKeyFindCoordinator;
}

int16_t ConsumerMetadataResponse::version() const
{
    return m_version;
}

int16_t ConsumerMetadataResponse::header_version() const
{
    return 0;
}

bool ConsumerMetadataResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 2;
}

KafkaVersion ConsumerMetadataResponse::required_version() const
{
    switch (m_version)
    {
    case 2:
        return V2_0_0_0;
    case 1:
        return V0_11_0_0;
    default:
        return V0_8_2_0;
    }
}