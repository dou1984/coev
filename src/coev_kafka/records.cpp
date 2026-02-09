#include "real_decoder.h"
#include "records.h"
#include <utility>
#include <vector>
#include <cassert>
#include <variant>
#include "errors.h"

Records::Records() : m_records_type(UnknownRecords)
{
}
Records::Records(MessageSet &&message_set)
{
    m_records_type = LegacyRecords;
    m_records = std::move(message_set);
}
Records::Records(RecordBatch &&batch)
{
    m_records_type = DefaultRecords;
    m_records = std::move(batch);
}

int Records::set_type_from_fields(bool &empty) const
{
    empty = false;
    if (m_records.index() == std::variant_npos)
    {
        empty = true;
        return 0;
    }
    m_records_type = DefaultRecords;
    if (std::holds_alternative<MessageSet>(m_records))
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
        return std::get<MessageSet>(m_records).encode(pe);
    }
    if (m_records_type == DefaultRecords)
    {
        return std::get<RecordBatch>(m_records).encode(pe);
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
        m_records = MessageSet();
        return std::get<MessageSet>(m_records).decode(pd);
    }
    if (m_records_type == DefaultRecords)
    {
        m_records = RecordBatch();
        return std::get<RecordBatch>(m_records).decode(pd);
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
        if (std::holds_alternative<MessageSet>(m_records))
        {
            _num_records = static_cast<int>(std::get<MessageSet>(m_records).m_messages.size());
        }
        return 0;
    }
    if (m_records_type == DefaultRecords)
    {
        if (std::holds_alternative<RecordBatch>(m_records))
        {
            _num_records = std::get<RecordBatch>(m_records).m_records.size();
        }
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
        if (std::holds_alternative<MessageSet>(m_records))
        {
            partial = std::get<MessageSet>(m_records).m_partial_trailing_message;
        }
        return 0;
    case DefaultRecords:
        if (std::holds_alternative<RecordBatch>(m_records))
        {
            partial = std::get<RecordBatch>(m_records).m_partial_trailing_record;
        }
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
        if (std::holds_alternative<RecordBatch>(m_records))
        {
            out = std::get<RecordBatch>(m_records).m_control;
        }
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
        if (std::holds_alternative<MessageSet>(m_records))
        {
            out = std::get<MessageSet>(m_records).m_overflow_message;
        }
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
        if (std::holds_alternative<RecordBatch>(m_records))
        {
            offset = std::get<RecordBatch>(m_records).last_offset() + 1;
        }
        return 0;
    default:
        offset = 0;
        return -1;
    }
}

int Records::get_control_record(ControlRecord &out) const
{
    if (!std::holds_alternative<RecordBatch>(m_records) || std::get<RecordBatch>(m_records).m_records.empty())
    {
        return -1;
    }

    auto &first_record = std::get<RecordBatch>(m_records).m_records[0];
    real_decoder key;
    key.m_raw = first_record.m_key;

    real_decoder value;
    value.m_raw = first_record.m_value;

    int err = out.decode(key, value);
    if (err != 0)
    {
        return err;
    }

    return 0;
}
