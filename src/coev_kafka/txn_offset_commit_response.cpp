#include "version.h"
#include "api_versions.h"
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "txn_offset_commit_response.h"

void TxnOffsetCommitResponse::set_version(int16_t v)
{
    m_version = v;
}

int TxnOffsetCommitResponse::encode(PEncoder &pe)
{
    pe.putDurationMs(m_throttle_time);
    pe.putArrayLength(static_cast<int32_t>(m_topics.size()));

    for (auto &topicPair : m_topics)
    {
        const std::string &topic = topicPair.first;
        auto &errors = topicPair.second;
        pe.putString(topic);
        pe.putArrayLength(static_cast<int32_t>(errors.size()));
        for (auto &partitionError : errors)
        {
            partitionError->encode(pe);
        }
    }
    return ErrNoError;
}

int TxnOffsetCommitResponse::decode(PDecoder &pd, int16_t version)
{
    m_version = version;

    auto err = pd.getDurationMs(m_throttle_time);
    if (err)
    {
        return err;
    }

    int32_t n;
    pd.getArrayLength(n);
    m_topics.clear();
    for (int i = 0; i < n; ++i)
    {
        std::string topic;
        pd.getString(topic);
        int32_t m;
        pd.getArrayLength(n);
        std::vector<std::shared_ptr<PartitionError>> errors(m);
        for (int j = 0; j < m; ++j)
        {
            auto err = std::make_shared<PartitionError>();
            err->decode(pd, version);
            errors[j] = err;
        }
        m_topics[topic] = std::move(errors);
    }
    return ErrNoError;
}

int16_t TxnOffsetCommitResponse::key() const
{
    return apiKeyTxnOffsetCommit;
}

int16_t TxnOffsetCommitResponse::version() const
{
    return m_version;
}

int16_t TxnOffsetCommitResponse::headerVersion() const
{
    return 0;
}

bool TxnOffsetCommitResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 2;
}

KafkaVersion TxnOffsetCommitResponse::required_version() const
{
    switch (m_version)
    {
    case 2:
        return V2_1_0_0;
    case 1:
        return V2_0_0_0;
    case 0:
        return V0_11_0_0;
    default:
        return V2_1_0_0;
    }
}

std::chrono::milliseconds TxnOffsetCommitResponse::throttleTime() const
{
    return m_throttle_time;
}