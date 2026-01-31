#include "version.h"
#include "fetch_request.h"
#include <stdexcept>

FetchRequest::FetchRequest()
    : m_version(0), m_max_wait_time(0), m_min_bytes(0), m_max_bytes(0), m_isolation(ReadUncommitted),
      m_session_id(0), m_session_epoch(0)
{
}
FetchRequest::FetchRequest(int16_t v)
    : m_version(v), m_max_bytes(0), m_isolation(ReadUncommitted), m_session_id(0), m_session_epoch(0)
{
}

void FetchRequest::set_version(int16_t v)
{
    m_version = v;
}

int FetchRequest::encode(packetEncoder &pe) const
{
    pe.putInt32(-1);
    pe.putInt32(m_max_wait_time);
    pe.putInt32(m_min_bytes);
    if (m_version >= 3)
    {
        pe.putInt32(m_max_bytes);
    }
    if (m_version >= 4)
    {
        pe.putInt8(static_cast<int8_t>(m_isolation));
    }
    if (m_version >= 7)
    {
        pe.putInt32(m_session_id);
        pe.putInt32(m_session_epoch);
    }

    int err = pe.putArrayLength(static_cast<int32_t>(m_blocks.size()));
    if (err != 0)
    {
        return err;
    }

    for (auto &[topic, partitions] : m_blocks)
    {
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

        for (auto &[partition, block] : partitions)
        {
            pe.putInt32(partition);
            err = block.encode(pe, m_version);
            if (err != 0)
            {
                return err;
            }
        }
    }

    if (m_version >= 7)
    {
        err = pe.putArrayLength(static_cast<int32_t>(m_forgotten.size()));
        if (err != 0)
        {
            return err;
        }

        for (auto &[topic, partitions] : m_forgotten)
        {
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

    if (m_version >= 11)
    {
        err = pe.putString(m_rack_id);
        if (err != 0)
        {
            return err;
        }
    }

    return 0;
}

int FetchRequest::decode(packetDecoder &pd, int16_t version)
{
    m_version = version;
    int err = 0;

    int32_t dummy;
    if ((err = pd.getInt32(dummy)) != 0)
    {
        return err;
    }
    if ((err = pd.getInt32(m_max_wait_time)) != 0)
    {
        return err;
    }
    if ((err = pd.getInt32(m_min_bytes)) != 0)
    {
        return err;
    }
    if (m_version >= 3)
    {
        if ((err = pd.getInt32(m_max_bytes)) != 0)
        {
            return err;
        }
    }
    if (m_version >= 4)
    {
        int8_t isolation_val;
        if ((err = pd.getInt8(isolation_val)) != 0)
        {
            return err;
        }
        m_isolation = static_cast<IsolationLevel>(isolation_val);
    }
    if (m_version >= 7)
    {
        if ((err = pd.getInt32(m_session_id)) != 0)
        {
            return err;
        }
        if ((err = pd.getInt32(m_session_epoch)) != 0)
        {
            return err;
        }
    }

    int32_t topic_count;
    if ((err = pd.getArrayLength(topic_count)) != 0)
    {
        return err;
    }
    if (topic_count == 0)
    {
        return 0;
    }

    m_blocks.clear();
    for (int i = 0; i < topic_count; ++i)
    {
        std::string topic;
        if ((err = pd.getString(topic)) != 0)
        {
            return err;
        }

        int32_t partition_count;
        if ((err = pd.getArrayLength(partition_count)) != 0)
        {
            return err;
        }

        auto &partition_map = m_blocks[topic];
        for (int j = 0; j < partition_count; ++j)
        {
            int32_t partition;
            if ((err = pd.getInt32(partition)) != 0)
            {
                return err;
            }
            if ((err = partition_map[partition].decode(pd, m_version)) != 0)
            {
                return err;
            }
        }
    }

    if (m_version >= 7)
    {
        int32_t forgottenCount;
        if ((err = pd.getArrayLength(forgottenCount)) != 0)
        {
            return err;
        }

        m_forgotten.clear();
        for (int i = 0; i < forgottenCount; ++i)
        {
            std::string topic;
            if ((err = pd.getString(topic)) != 0)
            {
                return err;
            }

            int32_t partition_count;
            if ((err = pd.getArrayLength(partition_count)) != 0)
            {
                return err;
            }

            if (partition_count < 0)
            {
                return -1;
            }

            auto &partitions = m_forgotten[topic];
            for (int j = 0; j < partition_count; ++j)
            {
                if ((err = pd.getInt32(partitions[j])) != 0)
                {
                    return err;
                }
            }
        }
    }

    if (m_version >= 11)
    {
        if ((err = pd.getString(m_rack_id)) != 0)
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
    return m_version;
}

int16_t FetchRequest::header_version() const
{
    return 1;
}

bool FetchRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 11;
}

KafkaVersion FetchRequest::required_version() const
{
    switch (m_version)
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

void FetchRequest::add_block(const std::string &topic, int32_t partition_id, int64_t fetch_offset, int32_t max_bytes, int32_t leader_epoch)
{

    auto &block = m_blocks[topic][partition_id];
    block.m_version = m_version;
    block.m_max_bytes = max_bytes;
    block.m_fetch_offset = fetch_offset;
    if (m_version >= 9)
    {
        block.m_current_leader_epoch = leader_epoch;
    }

    if (m_version >= 7 && m_forgotten.empty())
    {
        m_forgotten.clear();
    }
}