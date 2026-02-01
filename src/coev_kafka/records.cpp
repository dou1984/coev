#include "real_decoder.h"
#include "records.h"
#include <utility>
#include <vector>
#include "errors.h"

Records::Records() : m_records_type(UnknownRecords)
{
}
Records::Records(std::shared_ptr<MessageSet> message_set)
{

    m_records_type = LegacyRecords;
    m_message_set = message_set;
}
Records::Records(std::shared_ptr<RecordBatch> batch)
{
    m_records_type = DefaultRecords;
    m_record_batch = batch;
}

int Records::set_type_from_fields(bool &empty) const
{
    empty = false;
    if (m_message_set == nullptr && m_record_batch == nullptr)
    {
        empty = true;
        return 0;
    }
    if (m_message_set != nullptr && m_record_batch != nullptr)
    {
        return -1;
    }
    m_records_type = DefaultRecords;
    if (m_message_set != nullptr)
    {
        m_records_type = LegacyRecords;
    }

    return 0;
}

int Records::encode(packet_encoder &pe) const
{
    if (m_records_type == UnknownRecords)
    {
        bool empty;
        int err = set_type_from_fields(empty);
        if (err != 0 || empty)
        {
            return err;
        }
    }

    if (m_records_type == LegacyRecords)
    {
        if (m_message_set == nullptr)
        {
            return 0;
        }
        return m_message_set->encode(pe);
    }
    else if (m_records_type == DefaultRecords)
    {
        if (m_record_batch == nullptr)
        {
            return 0;
        }
        return m_record_batch->encode(pe);
    }
    return -1;
}

int Records::set_type_from_magic(packet_decoder &pd)
{
    int8_t magic;
    int err = magic_value(pd, magic);
    if (err != 0)
    {
        return err;
    }

    m_records_type = DefaultRecords;
    if (magic < 2)
    {
        m_records_type = LegacyRecords;
    }
    return 0;
}

int Records::decode(packet_decoder &pd)
{
    if (m_records_type == UnknownRecords)
    {
        int err = set_type_from_magic(pd);
        if (err != 0)
        {
            return err;
        }
    }
    if (m_records_type == LegacyRecords)
    {
        m_message_set = std::make_shared<MessageSet>();
        return m_message_set->decode(pd);
    }
    if (m_records_type == DefaultRecords)
    {
        m_record_batch = std::make_shared<RecordBatch>();
        return m_record_batch->decode(pd);
    }

    return -1;
}

int Records::num_records(int &_num_records) const
{
    _num_records = 0;
    if (m_records_type == UnknownRecords)
    {
        bool empty;
        int err = const_cast<Records *>(this)->set_type_from_fields(empty);
        if (err != 0 || empty)
        {
            return err;
        }
    }
    if (m_records_type == LegacyRecords)
    {
        if (m_message_set == nullptr)
        {
            return 0;
        }
        _num_records = static_cast<int>(m_message_set->m_messages.size());
        return 0;
    }
    if (m_records_type == DefaultRecords)
    {
        if (m_record_batch == nullptr)
        {
            return 0;
        }
        _num_records = m_record_batch->m_records.size();
        return 0;
    }
    return -1;
}

int Records::is_partial(bool &partial) const
{
    partial = false;
    if (m_records_type == UnknownRecords)
    {
        bool empty;
        int err = const_cast<Records *>(this)->set_type_from_fields(empty);
        if (err != 0 || empty)
        {
            return err;
        }
    }

    switch (m_records_type)
    {
    case UnknownRecords:
        return 0;
    case LegacyRecords:
        if (m_message_set == nullptr)
            return 0;
        partial = m_message_set->m_partial_trailing_message;
        return 0;
    case DefaultRecords:
        if (m_record_batch == nullptr)
            return 0;
        partial = m_record_batch->m_partial_trailing_record;
        return 0;
    default:
        return ErrUnknownRecordsType;
    }
}

int Records::is_control(bool &out) const
{
    out = false;
    if (m_records_type == UnknownRecords)
    {
        bool empty;
        int err = const_cast<Records *>(this)->set_type_from_fields(empty);
        if (err != 0 || empty)
        {
            return err;
        }
    }

    switch (m_records_type)
    {
    case LegacyRecords:
        return 0;
    case DefaultRecords:
        if (m_record_batch == nullptr)
            return 0;
        out = m_record_batch->m_control;
        return 0;
    default:
        return -1;
    }
}

int Records::is_overflow(bool &out)
{
    out = false;
    if (m_records_type == UnknownRecords)
    {
        bool empty;
        int err = set_type_from_fields(empty);
        if (err != 0 || empty)
        {
            return err;
        }
    }
    switch (m_records_type)
    {
    case UnknownRecords:
        return 0;
    case LegacyRecords:
        if (m_message_set == nullptr)
            return 0;
        out = m_message_set->m_overflow_message;
        return 0;
    case DefaultRecords:
        return 0;
    default:
        return -1;
    }
}

int Records::next_offset(int64_t &offset)
{
    offset = 0;
    switch (m_records_type)
    {
    case UnknownRecords:
    case LegacyRecords:
        return 0;
    case DefaultRecords:
        if (m_record_batch == nullptr)
            return 0;
        offset = m_record_batch->LastOffset() + 1;
        return 0;
    default:
        offset = 0;
        return -1;
    }
}

int Records::get_control_record(ControlRecord &out) const
{
    if (m_record_batch == nullptr || m_record_batch->m_records.empty())
    {
        return -1;
    }

    auto &firstRecord = m_record_batch->m_records[0];
    real_decoder keyDecoder;
    keyDecoder.m_raw = firstRecord->m_key;

    real_decoder valueDecoder;
    valueDecoder.m_raw = firstRecord->m_value;

    int err = out.decode(keyDecoder, valueDecoder);
    if (err != 0)
    {
        return err;
    }

    return 0;
}
