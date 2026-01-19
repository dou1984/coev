#include "version.h"
#include "api_versions.h"
#include "find_coordinator_response.h"
#include "broker.h"

void FindCoordinatorResponse::set_version(int16_t v)
{
    m_version = v;
}

int FindCoordinatorResponse::decode(packetDecoder &pd, int16_t version)
{
    if (version >= 1)
    {
        m_version = version;
        int err = pd.getDurationMs(m_throttle_time);
        if (err != 0)
        {
            return err;
        }
    }

    int err = pd.getKError(m_err);
    if (err != 0)
    {
        return err;
    }

    if (version >= 1)
    {
        err = pd.getNullableString(m_err_msg);
        if (err != 0)
        {
            return err;
        }
    }

    auto coordinator = std::make_shared<Broker>();
    err = coordinator->decode(pd, 0);
    if (err != 0)
    {
        return err;
    }

    if (coordinator->m_addr != ":0")
    {
        m_coordinator = coordinator;
    }
    else
    {
        m_coordinator.reset(); // addr == ":0" 视为空节点
    }

    return 0;
}

int FindCoordinatorResponse::encode(packetEncoder &pe)
{
    if (m_version >= 1)
    {
        pe.putDurationMs(m_throttle_time);
    }

    pe.putKError(m_err);

    if (m_version >= 1)
    {

        int err = pe.putNullableString(m_err_msg);
        if (err != 0)
            return err;
    }

    Broker *coord = m_coordinator ? m_coordinator.get() : std::addressof(coev::singleton<Broker>::instance());
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
    return m_version;
}

int16_t FindCoordinatorResponse::header_version() const
{
    return 0;
}

bool FindCoordinatorResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 3;
}

KafkaVersion FindCoordinatorResponse::required_version()  const
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

std::chrono::milliseconds FindCoordinatorResponse::throttle_time() const
{
    return m_throttle_time;
}