#include "version.h"
#include "fetch_request.h"
#include <stdexcept>

FetchRequest::FetchRequest()
    : Version(0), MaxWaitTime(0), MinBytes(0), MaxBytes(0), Isolation(ReadUncommitted),
      SessionID(0), SessionEpoch(0)
{
}
FetchRequest::FetchRequest(int16_t v)
    : Version(v), MaxBytes(0), Isolation(ReadUncommitted), SessionID(0), SessionEpoch(0)
{
}

void FetchRequest::setVersion(int16_t v)
{
    Version = v;
}

int FetchRequest::encode(PEncoder &pe)
{
    pe.putInt32(-1);
    pe.putInt32(MaxWaitTime);
    pe.putInt32(MinBytes);
    if (Version >= 3)
    {
        pe.putInt32(MaxBytes);
    }
    if (Version >= 4)
    {
        pe.putInt8(static_cast<int8_t>(Isolation));
    }
    if (Version >= 7)
    {
        pe.putInt32(SessionID);
        pe.putInt32(SessionEpoch);
    }

    int err = pe.putArrayLength(static_cast<int32_t>(blocks.size()));
    if (err != 0)
    {
        return err;
    }

    for (auto &topic_pair : blocks)
    {
        const std::string &topic = topic_pair.first;
        auto &partitions = topic_pair.second;

        err = pe.putString(topic);
        if (err != 0)
        {
            return err;
        }

        err = pe.putArrayLength(static_cast<int32_t>(partitions.size()));
        if (err != 0)
        {
            return err;
        }

        for (auto &partition_pair : partitions)
        {
            int32_t partition = partition_pair.first;
            auto &block = partition_pair.second;

            pe.putInt32(partition);
            err = block->encode(pe, Version);
            if (err != 0)
            {
                return err;
            }
        }
    }

    if (Version >= 7)
    {
        err = pe.putArrayLength(static_cast<int32_t>(forgotten.size()));
        if (err != 0)
        {
            return err;
        }

        for (auto &topic_pair : forgotten)
        {
            const std::string &topic = topic_pair.first;
            const std::vector<int32_t> &partitions = topic_pair.second;

            err = pe.putString(topic);
            if (err != 0)
            {
                return err;
            }

            err = pe.putArrayLength(static_cast<int32_t>(partitions.size()));
            if (err != 0)
            {
                return err;
            }

            for (int32_t partition : partitions)
            {
                pe.putInt32(partition);
            }
        }
    }

    if (Version >= 11)
    {
        err = pe.putString(RackID);
        if (err != 0)
        {
            return err;
        }
    }

    return 0;
}

int FetchRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;
    int err = 0;

    int32_t dummy;
    if ((err = pd.getInt32(dummy)) != 0)
    {
        return err;
    }
    if ((err = pd.getInt32(MaxWaitTime)) != 0)
    {
        return err;
    }
    if ((err = pd.getInt32(MinBytes)) != 0)
    {
        return err;
    }
    if (Version >= 3)
    {
        if ((err = pd.getInt32(MaxBytes)) != 0)
        {
            return err;
        }
    }
    if (Version >= 4)
    {
        int8_t isolation_val;
        if ((err = pd.getInt8(isolation_val)) != 0)
        {
            return err;
        }
        Isolation = static_cast<IsolationLevel>(isolation_val);
    }
    if (Version >= 7)
    {
        if ((err = pd.getInt32(SessionID)) != 0)
        {
            return err;
        }
        if ((err = pd.getInt32(SessionEpoch)) != 0)
        {
            return err;
        }
    }

    int32_t topicCount;
    if ((err = pd.getArrayLength(topicCount)) != 0)
    {
        return err;
    }
    if (topicCount == 0)
    {
        return 0;
    }

    blocks.clear();
    for (int i = 0; i < topicCount; ++i)
    {
        std::string topic;
        if ((err = pd.getString(topic)) != 0)
        {
            return err;
        }

        int32_t partitionCount;
        if ((err = pd.getArrayLength(partitionCount)) != 0)
        {
            return err;
        }

        std::map<int32_t, std::shared_ptr<FetchRequestBlock>> partitionMap;
        for (int j = 0; j < partitionCount; ++j)
        {
            int32_t partition;
            if ((err = pd.getInt32(partition)) != 0)
            {
                return err;
            }

            auto fetchBlock = std::make_shared<FetchRequestBlock>();
            if ((err = fetchBlock->decode(pd, Version)) != 0)
            {
                return err;
            }
            partitionMap[partition] = fetchBlock;
        }
        blocks[topic] = std::move(partitionMap);
    }

    if (Version >= 7)
    {
        int32_t forgottenCount;
        if ((err = pd.getArrayLength(forgottenCount)) != 0)
        {
            return err;
        }

        forgotten.clear();
        for (int i = 0; i < forgottenCount; ++i)
        {
            std::string topic;
            if ((err = pd.getString(topic)) != 0)
            {
                return err;
            }

            int32_t partitionCount;
            if ((err = pd.getArrayLength(partitionCount)) != 0)
            {
                return err;
            }

            if (partitionCount < 0)
            {
                return -1;
            }

            std::vector<int32_t> partitions(partitionCount);
            for (int j = 0; j < partitionCount; ++j)
            {
                if ((err = pd.getInt32(partitions[j])) != 0)
                {
                    return err;
                }
            }
            forgotten[topic] = std::move(partitions);
        }
    }

    if (Version >= 11)
    {
        if ((err = pd.getString(RackID)) != 0)
        {
            return err;
        }
    }

    return 0;
}

int16_t FetchRequest::key() const
{
    return apiKeyFetch;
}

int16_t FetchRequest::version() const
{
    return Version;
}

int16_t FetchRequest::headerVersion() const
{
    return 1;
}

bool FetchRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 11;
}

KafkaVersion FetchRequest::requiredVersion() const
{
    switch (Version)
    {
    case 11:
        return V2_3_0_0;
    case 9:
    case 10:
        return V2_1_0_0;
    case 8:
        return V2_0_0_0;
    case 7:
        return V1_1_0_0;
    case 6:
        return V1_0_0_0;
    case 4:
    case 5:
        return V0_11_0_0;
    case 3:
        return V0_10_1_0;
    case 2:
        return V0_10_0_0;
    case 1:
        return V0_9_0_0;
    case 0:
        return V0_8_2_0;
    default:
        return V2_3_0_0;
    }
}

void FetchRequest::AddBlock(const std::string &topic, int32_t partitionID, int64_t fetchOffset, int32_t maxBytes, int32_t leaderEpoch)
{
    if (blocks.find(topic) == blocks.end())
    {
        blocks[topic] = std::map<int32_t, std::shared_ptr<FetchRequestBlock>>();
    }

    auto tmp = std::make_shared<FetchRequestBlock>();
    tmp->Version = Version;
    tmp->maxBytes = maxBytes;
    tmp->fetchOffset = fetchOffset;
    if (Version >= 9)
    {
        tmp->currentLeaderEpoch = leaderEpoch;
    }

    blocks[topic][partitionID] = tmp;

    if (Version >= 7 && forgotten.empty())
    {
        forgotten = std::map<std::string, std::vector<int32_t>>();
    }
}