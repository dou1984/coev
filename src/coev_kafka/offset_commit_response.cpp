#include "version.h"
#include "offset_commit_response.h"
#include <stdexcept>

OffsetCommitResponse::OffsetCommitResponse()
    : Version(0)
{
}

void OffsetCommitResponse::setVersion(int16_t v)
{
    Version = v;
}

void OffsetCommitResponse::AddError(const std::string &topic, int32_t partition, KError kerror)
{
    auto &partitions = Errors[topic];
    partitions[partition] = kerror;
}

int OffsetCommitResponse::encode(PEncoder &pe)
{
    if (Version >= 3)
    {
        pe.putDurationMs(ThrottleTime);
    }

    pe.putArrayLength(static_cast<int32_t>(Errors.size()));
    for (auto &topicEntry : Errors)
    {
        auto &topic = topicEntry.first;
        auto &partitions = topicEntry.second;
        pe.putString(topic);
        pe.putArrayLength(static_cast<int32_t>(partitions.size()));
        for (auto &partitionEntry : partitions)
        {
            pe.putInt32(partitionEntry.first);
            pe.putKError(partitionEntry.second);
        }
    }
    return ErrNoError;
}

int OffsetCommitResponse::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    int err = ErrNoError;
    if (version >= 3)
    {

        err = pd.getDurationMs(ThrottleTime);
        if (err != ErrNoError)
        {
            return err;
        }
    }

    int32_t numTopics;
    err = pd.getArrayLength(numTopics);
    if (err != ErrNoError)
    {
        return err;
    }
    if (numTopics <= 0)
    {
        return ErrNoError;
    }

    Errors.clear();
    Errors.reserve(numTopics);
    for (int i = 0; i < numTopics; ++i)
    {
        std::string name;
        err = pd.getString(name);
        if (err != ErrNoError)
        {
            return err;
        }
        int32_t numErrors;
        err = pd.getArrayLength(numErrors);
        if (err != ErrNoError)
        {
            return err;
        }
        auto &partitionMap = Errors[name];
        partitionMap.reserve(numErrors);
        for (int j = 0; j < numErrors; ++j)
        {
            int32_t id;
            err = pd.getInt32(id);
            if (err != ErrNoError)
            {
                return err;
            }
            KError e;
            err = pd.getKError(e);
            if (err != ErrNoError)
            {
                return err;
            }
            partitionMap[id] = e;
        }
    }
    return 0;
}

int16_t OffsetCommitResponse::key() const
{
    return apiKeyOffsetCommit;
}

int16_t OffsetCommitResponse::version() const
{
    return Version;
}

int16_t OffsetCommitResponse::headerVersion() const
{
    return 0;
}

bool OffsetCommitResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 7;
}

KafkaVersion OffsetCommitResponse::requiredVersion() const
{
    switch (Version)
    {
    case 7:
        return V2_3_0_0;
    case 5:
    case 6:
        return V2_1_0_0;
    case 4:
        return V2_0_0_0;
    case 3:
        return V0_11_0_0;
    case 2:
        return V0_9_0_0;
    case 0:
    case 1:
        return V0_8_2_0;
    default:
        return V2_4_0_0;
    }
}

std::chrono::milliseconds OffsetCommitResponse::throttleTime() const
{
    return ThrottleTime;
}