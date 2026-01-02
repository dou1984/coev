#include "version.h"
#include "add_partitions_to_txn_response.h"
#include "partition_error.h"
#include "api_versions.h"

void AddPartitionsToTxnResponse::setVersion(int16_t v)
{
    Version = v;
}

int AddPartitionsToTxnResponse::encode(PEncoder &pe)
{
    pe.putDurationMs(ThrottleTime);

    if (pe.putArrayLength(static_cast<int32_t>(Errors.size())) != 0)
    {
        return -1;
    }

    for (auto &topicEntry : Errors)
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

int AddPartitionsToTxnResponse::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    int err;
    if ((err = pd.getDurationMs(ThrottleTime)) != 0)
    {
        return err;
    }

    int32_t n;
    if ((err = pd.getArrayLength(n)) != 0)
    {
        return err;
    }

    Errors.clear();
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

        Errors[topic] = std::move(partitionErrors);
    }

    return 0;
}

int16_t AddPartitionsToTxnResponse::key() const
{
    return apiKeyAddPartitionsToTxn;
}

int16_t AddPartitionsToTxnResponse::version() const
{
    return Version;
}

int16_t AddPartitionsToTxnResponse::headerVersion() const
{
    return 0;
}

bool AddPartitionsToTxnResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 2;
}

KafkaVersion AddPartitionsToTxnResponse::requiredVersion() const
{
    switch (Version)
    {
    case 2:
        return V2_7_0_0;
    case 1:
        return V2_0_0_0;
    default:
        return V0_11_0_0;
    }
}

std::chrono::milliseconds AddPartitionsToTxnResponse::throttleTime() const
{
    return ThrottleTime;
}