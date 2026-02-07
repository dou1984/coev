#include "version.h"
#include "add_partitions_to_txn_response.h"
#include "partition_error.h"
#include "api_versions.h"

void AddPartitionsToTxnResponse::set_version(int16_t v)
{
    m_version = v;
}

int AddPartitionsToTxnResponse::encode(packet_encoder &pe) const
{
    pe.putDurationMs(m_throttle_time);

    if (pe.putArrayLength(static_cast<int32_t>(m_errors.size())) != 0)
    {
        return -1;
    }

    for (auto &[topic, errors] : m_errors)
    {

        if (pe.putString(topic) != 0)
        {
            return -1;
        }

        if (pe.putArrayLength(static_cast<int32_t>(errors.size())) != 0)
        {
            return -1;
        }

        for (auto &partition_errors : errors)
        {
            if (partition_errors.encode(pe) != 0)
            {
                return -1;
            }
        }
    }

    return 0;
}

int AddPartitionsToTxnResponse::decode(packet_decoder &pd, int16_t version)
{
    m_version = version;

    int err;
    if ((err = pd.getDurationMs(m_throttle_time)) != 0)
    {
        return err;
    }

    int32_t n;
    if ((err = pd.getArrayLength(n)) != 0)
    {
        return err;
    }

    m_errors.clear();
    for (int32_t i = 0; i < n; ++i)
    {
        std::string topic;
        if ((err = pd.getString(topic)) != 0)
        {
            return err;
        }

        int32_t m;
        if ((err = pd.getArrayLength(m)) != 0)
        {
            return err;
        }

        auto &partition_errors = m_errors[topic];
        partition_errors.reserve(m);
        for (int32_t j = 0; j < m; ++j)
        {
            if ((err = partition_errors[j].decode(pd, version)) != 0)
            {
                return err;
            }
        }
    }

    return 0;
}

int16_t AddPartitionsToTxnResponse::key() const
{
    return apiKeyAddPartitionsToTxn;
}

int16_t AddPartitionsToTxnResponse::version() const
{
    return m_version;
}

int16_t AddPartitionsToTxnResponse::header_version() const
{
    return 0;
}

bool AddPartitionsToTxnResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 2;
}

KafkaVersion AddPartitionsToTxnResponse::required_version() const
{
    switch (m_version)
    {
    case 2:
        return V2_7_0_0;
    case 1:
        return V2_0_0_0;
    default:
        return V0_11_0_0;
    }
}

std::chrono::milliseconds AddPartitionsToTxnResponse::throttle_time() const
{
    return m_throttle_time;
}