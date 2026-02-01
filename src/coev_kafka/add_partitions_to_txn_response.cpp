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

    for (auto &topicEntry : m_errors)
    {
        const std::string &topic = topicEntry.first;
        auto &errorList = topicEntry.second;

        if (pe.putString(topic) != 0)
        {
            return -1;
        }

        if (pe.putArrayLength(static_cast<int32_t>(errorList.size())) != 0)
        {
            return -1;
        }

        for (auto &partitionError : errorList)
        {
            if (partitionError->encode(pe) != 0)
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

        std::vector<std::shared_ptr<PartitionError>> partitionErrors;
        partitionErrors.reserve(m);
        for (int32_t j = 0; j < m; ++j)
        {
            auto partErr = std::make_shared<PartitionError>();
            if ((err = partErr->decode(pd, version)) != 0)
            {
                return err;
            }
            partitionErrors.emplace_back(partErr);
        }

        m_errors[topic] = std::move(partitionErrors);
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