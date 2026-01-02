#include "version.h"
#include "api_versions.h"
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "txn_offset_commit_response.h"

void TxnOffsetCommitResponse::setVersion(int16_t v)
{
    Version = v;
}

int TxnOffsetCommitResponse::encode(PEncoder &pe)
{
    pe.putDurationMs(ThrottleTime);
    pe.putArrayLength(static_cast<int32_t>(Topics.size()));

    for (auto &topicPair : Topics)
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
    Version = version;

    auto err = pd.getDurationMs(ThrottleTime);
    if (err)
    {
        return err;
    }

    int32_t n;
    pd.getArrayLength(n);
    Topics.clear();
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
        Topics[topic] = std::move(errors);
    }
    return ErrNoError;
}

int16_t TxnOffsetCommitResponse::key() const
{
    return apiKeyTxnOffsetCommit;
}

int16_t TxnOffsetCommitResponse::version() const
{
    return Version;
}

int16_t TxnOffsetCommitResponse::headerVersion() const
{
    return 0;
}

bool TxnOffsetCommitResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 2;
}

KafkaVersion TxnOffsetCommitResponse::requiredVersion() const
{
    switch (Version)
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
    return ThrottleTime;
}