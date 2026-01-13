#include "version.h"
#include "find_coordinator_request.h"
#include "api_versions.h"

FindCoordinatorRequest::FindCoordinatorRequest()
    : m_version(0), m_coordinator_type(CoordinatorGroup) {}

void FindCoordinatorRequest::set_version(int16_t v)
{
    m_version = v;
}

int FindCoordinatorRequest::encode(packetEncoder &pe)
{
    int err = pe.putString(m_coordinator_key);
    if (err != 0)
        return err;

    if (m_version >= 1)
    {
        pe.putInt8(static_cast<int8_t>(m_coordinator_type));
    }

    return 0;
}

int FindCoordinatorRequest::decode(packetDecoder &pd, int16_t version)
{
    int err = pd.getString(m_coordinator_key);
    if (err != 0)
        return err;

    if (version >= 1)
    {
        m_version = version;
        int8_t coordinatorType;
        err = pd.getInt8(coordinatorType);
        if (err != 0)
            return err;
        m_coordinator_type = static_cast<CoordinatorType>(coordinatorType);
    }

    return 0;
}

int16_t FindCoordinatorRequest::key() const
{
    return apiKeyFindCoordinator;
}

int16_t FindCoordinatorRequest::version() const
{
    return m_version;
}

int16_t FindCoordinatorRequest::header_version() const
{
    return 1;
}

bool FindCoordinatorRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 2;
}

KafkaVersion FindCoordinatorRequest::required_version() const
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