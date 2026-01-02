#include "consumer_group_members.h"

int ConsumerGroupMemberMetadata::encode(PEncoder &pe)
{
    pe.putInt16(Version);

    if (int err = pe.putStringArray(Topics); err != 0)
    {
        return err;
    }

    if (int err = pe.putBytes(UserData); err != 0)
    {
        return err;
    }

    if (Version >= 1)
    {
        if (int err = pe.putArrayLength(OwnedPartitions.size()); err != 0)
        {
            return err;
        }
        for (auto &op : OwnedPartitions)
        {
            if (int err = op->encode(pe); err != 0)
            {
                return err;
            }
        }
    }

    if (Version >= 2)
    {
        pe.putInt32(GenerationId);
    }

    if (Version >= 3)
    {
        if (int err = pe.putNullableString(RackId); err != 0)
        {
            return err;
        }
    }

    return 0;
}

int ConsumerGroupMemberMetadata::decode(PDecoder &pd)
{
    int err;
    if ((err = pd.getInt16(Version)) != 0)
    {
        return err;
    }

    if ((err = pd.getStringArray(Topics)) != 0)
    {
        return err;
    }

    if ((err = pd.getBytes(UserData)) != 0)
    {
        return err;
    }

    if (Version >= 1)
    {
        int n;
        if ((err = pd.getArrayLength(n)) != 0)
        {

            if (err == -2)
            {
                return 0;
            }
            return err;
        }
        if (n > 0)
        {
            OwnedPartitions.resize(n);
            for (int i = 0; i < n; i++)
            {
                OwnedPartitions[i] = std::make_shared<OwnedPartition>();
                if ((err = OwnedPartitions[i]->decode(pd)) != 0)
                {
                    return err;
                }
            }
        }
    }

    if (Version >= 2)
    {
        if ((err = pd.getInt32(GenerationId)) != 0)
        {
            return err;
        }
    }

    if (Version >= 3)
    {
        if ((err = pd.getNullableString(RackId)) != 0)
        {
            return err;
        }
    }

    return 0;
}

int OwnedPartition::encode(PEncoder &pe)
{
    if (int err = pe.putString(topic); err != 0)
    {
        return err;
    }
    if (int err = pe.putInt32Array(partitions); err != 0)
    {
        return err;
    }
    return 0;
}

int OwnedPartition::decode(PDecoder &pd)
{
    int err;
    if ((err = pd.getString(topic)) != 0)
    {
        return err;
    }
    if ((err = pd.getInt32Array(partitions)) != 0)
    {
        return err;
    }

    return 0;
}

int ConsumerGroupMemberAssignment::encode(PEncoder &pe)
{
    pe.putInt16(Version);

    if (int err = pe.putArrayLength(Topics.size()); err != 0)
    {
        return err;
    }

    for (auto &pair : Topics)
    {
        if (int err = pe.putString(pair.first); err != 0)
        {
            return err;
        }
        if (int err = pe.putInt32Array(pair.second); err != 0)
        {
            return err;
        }
    }

    if (int err = pe.putBytes(UserData); err != 0)
    {
        return err;
    }

    return 0;
}

int ConsumerGroupMemberAssignment::decode(PDecoder &pd)
{
    int err;
    if ((err = pd.getInt16(Version)) != 0)
    {
        return err;
    }

    int topic_len;
    if ((err = pd.getArrayLength(topic_len)) != 0)
    {
        return err;
    }

    for (int i = 0; i < topic_len; i++)
    {
        std::string topic;
        if ((err = pd.getString(topic)) != 0)
        {
            return err;
        }
        if ((err = pd.getInt32Array(Topics[topic])) != 0)
        {
            return err;
        }
    }

    if ((err = pd.getBytes(UserData)) != 0)
    {
        return err;
    }

    return 0;
}