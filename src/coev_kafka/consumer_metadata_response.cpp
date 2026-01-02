#include "consumer_metadata_response.h"
#include "find_coordinator_response.h"
#include "broker.h"
#include "version.h"
#include <string>
#include <sstream>
#include <stdexcept>

void ConsumerMetadataResponse::setVersion(int16_t v)
{
    Version = v;
}

int ConsumerMetadataResponse::decode(PDecoder &pd, int16_t version)
{
    auto tmp = std::make_shared<FindCoordinatorResponse>();

    if (int err = tmp->decode(pd, version); err != 0)
    {
        return err;
    }

    Err = tmp->Err;
    Coordinator = tmp->Coordinator;

    if (Coordinator == nullptr)
    {
        return 0;
    }

    std::string addr = Coordinator->Addr();
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
        CoordinatorID = Coordinator->ID();
        CoordinatorHost = host;
        CoordinatorPort = static_cast<int32_t>(port);
    }
    catch (...)
    {
        return -1;
    }

    return 0;
}

int ConsumerMetadataResponse::encode(PEncoder &pe)
{
    if (Coordinator == nullptr)
    {
        Coordinator = std::make_shared<Broker>();
        Coordinator->m_ID = CoordinatorID;
        std::ostringstream oss;
        oss << CoordinatorHost << ":" << CoordinatorPort;
        Coordinator->m_Addr = oss.str();
    }

    FindCoordinatorResponse tmp;
    tmp.Version = Version;
    tmp.Err = Err;
    tmp.Coordinator = Coordinator;

    if (int err = tmp.encode(pe); err != 0)
    {
        return err;
    }

    return 0;
}

int16_t ConsumerMetadataResponse::key() const
{
#include "api_versions.h"
    return apiKeyFindCoordinator;
}

int16_t ConsumerMetadataResponse::version() const
{
    return Version;
}

int16_t ConsumerMetadataResponse::headerVersion() const
{
    return 0;
}

bool ConsumerMetadataResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 2;
}

KafkaVersion ConsumerMetadataResponse::requiredVersion() const
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