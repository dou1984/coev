#include "consumer_group_members.h"

int ConsumerGroupMemberMetadata::encode(packetEncoder &pe) const
{
    pe.putInt16(m_version);

    if (int err = pe.putStringArray(m_topics); err != 0)
    {
        return err;
    }

    if (int err = pe.putBytes(m_user_data); err != 0)
    {
        return err;
    }

    if (m_version >= 1)
    {
        if (int err = pe.putArrayLength(m_owned_partitions.size()); err != 0)
        {
            return err;
        }
        for (auto &op : m_owned_partitions)
        {
            if (int err = op->encode(pe); err != 0)
            {
                return err;
            }
        }
    }

    if (m_version >= 2)
    {
        pe.putInt32(m_generation_id);
    }

    if (m_version >= 3)
    {
        if (int err = pe.putNullableString(m_rack_id); err != 0)
        {
            return err;
        }
    }

    return 0;
}

int ConsumerGroupMemberMetadata::decode(packetDecoder &pd)
{
    int err;
    if ((err = pd.getInt16(m_version)) != 0)
    {
        return err;
    }

    if ((err = pd.getStringArray(m_topics)) != 0)
    {
        return err;
    }

    if ((err = pd.getBytes(m_user_data)) != 0)
    {
        return err;
    }

    if (m_version >= 1)
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
            m_owned_partitions.resize(n);
            for (int i = 0; i < n; i++)
            {
                m_owned_partitions[i] = std::make_shared<OwnedPartition>();
                if ((err = m_owned_partitions[i]->decode(pd)) != 0)
                {
                    return err;
                }
            }
        }
    }

    if (m_version >= 2)
    {
        if ((err = pd.getInt32(m_generation_id)) != 0)
        {
            return err;
        }
    }

    if (m_version >= 3)
    {
        if ((err = pd.getNullableString(m_rack_id)) != 0)
        {
            return err;
        }
    }

    return 0;
}

int OwnedPartition::encode(packetEncoder &pe) const
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

int OwnedPartition::decode(packetDecoder &pd)
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

int ConsumerGroupMemberAssignment::encode(packetEncoder &pe) const
{
    pe.putInt16(m_version);

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

int ConsumerGroupMemberAssignment::decode(packetDecoder &pd)
{
    int err;
    if ((err = pd.getInt16(m_version)) != 0)
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