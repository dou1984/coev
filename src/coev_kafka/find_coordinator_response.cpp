#include "version.h"
#include "api_versions.h"
#include "find_coordinator_response.h"
#include "broker.h"

auto NoNode = std::make_shared<Broker>(-1, ":-1");

void FindCoordinatorResponse::setVersion(int16_t v)
{
    Version = v;
}

int FindCoordinatorResponse::decode(PDecoder &pd, int16_t version)
{
    if (version >= 1)
    {
        Version = version;
        int err = pd.getDurationMs(ThrottleTime);
        if (err != 0)
        {
            return err;
        }
    }

    int err = pd.getKError(Err);
    if (err != 0)
    {
        return err;
    }

    if (version >= 1)
    {
        err = pd.getNullableString(ErrMsg);
        if (err != 0)
        {
            return err;
        }
    }

    auto coordinator = std::shared_ptr<Broker>();
    err = coordinator->decode(pd, 0);
    if (err != 0)
    {
        return err;
    }

    if (coordinator->m_Addr != ":0")
    {
        Coordinator = coordinator;
    }
    else
    {
        Coordinator.reset(); // addr == ":0" 视为空节点
    }

    return 0;
}

int FindCoordinatorResponse::encode(PEncoder &pe)
{
    if (Version >= 1)
    {
        pe.putDurationMs(ThrottleTime);
    }

    pe.putKError(Err);

    if (Version >= 1)
    {

        int err = pe.putNullableString(ErrMsg);
        if (err != 0)
            return err;
    }

    Broker *coord = Coordinator ? Coordinator.get() : NoNode.get();
    int err = coord->encode(pe, 0); // 硬编码使用 Broker 编码版本 0
    if (err != 0)
        return err;

    return 0;
}

int16_t FindCoordinatorResponse::key() const
{
    return apiKeyFindCoordinator;
}

int16_t FindCoordinatorResponse::version() const
{
    return Version;
}

int16_t FindCoordinatorResponse::headerVersion() const
{
    return 0;
}

bool FindCoordinatorResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 2;
}

KafkaVersion FindCoordinatorResponse::requiredVersion() const
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

std::chrono::milliseconds FindCoordinatorResponse::throttleTime() const
{
    return ThrottleTime;
}