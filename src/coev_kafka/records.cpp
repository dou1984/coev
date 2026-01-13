#include "real_decoder.h"
#include "records.h"
#include <utility>
#include <vector>
#include "errors.h"

Records::Records() : m_records_type(unknownRecords)
{
}

std::shared_ptr<Records> Records::NewLegacyRecords(std::shared_ptr<MessageSet> msgSet)
{
    auto r = std::make_shared<Records>();
    r->m_records_type = legacyRecords;
    r->m_msg_set = std::move(msgSet);
    return r;
}

std::shared_ptr<Records> Records::NewDefaultRecords(std::shared_ptr<RecordBatch> batch)
{
    auto r = std::make_shared<Records>();
    r->m_records_type = defaultRecords;
    r->m_record_batch = std::move(batch);
    return r;
}

int Records::SetTypeFromFields(bool &empty)
{
    empty = false;
    if (m_msg_set == nullptr && m_record_batch == nullptr)
    {
        empty = true;
        return 0;
    }
    if (m_msg_set != nullptr && m_record_batch != nullptr)
    {
        return -1;
    }
    m_records_type = defaultRecords;
    if (m_msg_set != nullptr)
    {
        m_records_type = legacyRecords;
    }

    return 0;
}

int Records::encode(packetEncoder &pe)
{
    if (m_records_type == unknownRecords)
    {
        bool empty;
        int err = SetTypeFromFields(empty);
        if (err != 0 || empty)
        {
            return err;
        }
    }

    switch (m_records_type)
    {
    case legacyRecords:
        if (m_msg_set == nullptr)
            return 0;
        return m_msg_set->encode(pe);
    case defaultRecords:
        if (m_record_batch == nullptr)
            return 0;
        return m_record_batch->encode(pe);
    default:
        return -1; // unknown type
    }
}

int Records::SetTypeFromMagic(packetDecoder &pd)
{
    int8_t magic;
    int err = magicValue(pd, magic);
    if (err != 0)
    {
        return err;
    }

    m_records_type = defaultRecords;
    if (magic < 2)
    {
        m_records_type = legacyRecords;
    }
    return 0;
}

int Records::decode(packetDecoder &pd)
{
    if (m_records_type == unknownRecords)
    {
        int err = SetTypeFromMagic(pd);
        if (err != 0)
        {
            return err;
        }
    }

    switch (m_records_type)
    {
    case legacyRecords:
    {
        m_msg_set = std::make_shared<MessageSet>();
        return m_msg_set->decode(pd);
    }
    case defaultRecords:
    {
        m_record_batch = std::make_shared<RecordBatch>();
        return m_record_batch->decode(pd);
    }
    default:
        return -1;
    }
}

int Records::numRecords(int &numRecords)
{
    numRecords = 0;
    if (m_records_type == unknownRecords)
    {
        bool empty;
        int err = SetTypeFromFields(empty);
        if (err != 0 || empty)
        {
            return err;
        }
    }

    switch (m_records_type)
    {
    case legacyRecords:
        if (m_msg_set == nullptr)
            return 0;
        numRecords = static_cast<int>(m_msg_set->m_messages.size());
        return 0;
    case defaultRecords:
        if (m_record_batch == nullptr)
            return 0;
        numRecords = m_record_batch->m_records.size();
        return 0;
    default:
        return -1;
    }
}

int Records::isPartial(bool &partial)
{
    partial = false;
    if (m_records_type == unknownRecords)
    {
        bool empty;
        int err = SetTypeFromFields(empty);
        if (err != 0 || empty)
        {
            return err;
        }
    }

    switch (m_records_type)
    {
    case unknownRecords:
        return 0;
    case legacyRecords:
        if (m_msg_set == nullptr)
            return 0;
        partial = m_msg_set->m_partial_trailing_message;
        return 0;
    case defaultRecords:
        if (m_record_batch == nullptr)
            return 0;
        partial = m_record_batch->m_partial_trailing_record;
        return 0;
    default:
        return ErrUnknownRecordsType;
    }
}

int Records::isControl(bool &out)
{
    out = false;
    if (m_records_type == unknownRecords)
    {
        bool empty;
        int err = SetTypeFromFields(empty);
        if (err != 0 || empty)
        {
            return err;
        }
    }

    switch (m_records_type)
    {
    case legacyRecords:
        return 0;
    case defaultRecords:
        if (m_record_batch == nullptr)
            return 0;
        out = m_record_batch->m_control;
        return 0;
    default:
        return -1;
    }
}

int Records::isOverflow(bool &out)
{
    out = false;
    if (m_records_type == unknownRecords)
    {
        bool empty;
        int err = SetTypeFromFields(empty);
        if (err != 0 || empty)
        {
            return err;
        }
    }
    switch (m_records_type)
    {
    case unknownRecords:
        return 0;
    case legacyRecords:
        if (m_msg_set == nullptr)
            return 0;
        out = m_msg_set->m_overflow_message;
        return 0;
    case defaultRecords:
        return 0;
    default:
        return -1;
    }
}

int Records::nextOffset(int64_t &offset)
{
    offset = 0;
    switch (m_records_type)
    {
    case unknownRecords:
    case legacyRecords:
        return 0;
    case defaultRecords:
        if (m_record_batch == nullptr)
            return 0;
        offset = m_record_batch->LastOffset() + 1;
        return 0;
    default:
        offset = 0;
        return -1;
    }
}

int Records::getControlRecord(ControlRecord &out)
{
    if (m_record_batch == nullptr || m_record_batch->m_records.empty())
    {
        return -1;
    }

    auto &firstRecord = m_record_batch->m_records[0];
    realDecoder keyDecoder;
    keyDecoder.m_raw = firstRecord->m_key;

    realDecoder valueDecoder;
    valueDecoder.m_raw = firstRecord->m_value;

    int err = out.decode(keyDecoder, valueDecoder);
    if (err != 0)
    {
        return err;
    }

    return 0;
}
