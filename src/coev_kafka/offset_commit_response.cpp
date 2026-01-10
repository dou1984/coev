#include "version.h"
#include "offset_commit_response.h"
#include <stdexcept>

OffsetCommitResponse::OffsetCommitResponse()
    : m_version(0)
{
}

void OffsetCommitResponse::set_version(int16_t v)
{
    m_version = v;
}

void OffsetCommitResponse::AddError(const std::string &topic, int32_t partition, KError kerror)
{
    auto &partitions = m_errors[topic];
    partitions[partition] = kerror;
}

int OffsetCommitResponse::encode(PEncoder &pe)
{
    if (m_version >= 3)
    {
        pe.putDurationMs(m_throttle_time);
    }

    pe.putArrayLength(static_cast<int32_t>(m_errors.size()));
    for (auto &topicEntry : m_errors)
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
    m_version = version;

    int err = ErrNoError;
    if (version >= 3)
    {

        err = pd.getDurationMs(m_throttle_time);
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

    m_errors.clear();
    m_errors.reserve(numTopics);
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
        auto &partitionMap = m_errors[name];
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
    return m_version;
}

int16_t OffsetCommitResponse::headerVersion() const
{
    return 0;
}

bool OffsetCommitResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 7;
}

KafkaVersion OffsetCommitResponse::required_version() const
{
    switch (m_version)
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
    return m_throttle_time;
}