#include "real_decoder.h"
#include "records.h"
#include <utility>
#include <vector>
#include "errors.h"

Records::Records() : RecordsType(unknownRecords)
{
}

std::shared_ptr<Records> Records::NewLegacyRecords(std::shared_ptr<MessageSet> msgSet)
{
    auto r = std::make_shared<Records>();
    r->RecordsType = legacyRecords;
    r->MsgSet = std::move(msgSet);
    return r;
}

std::shared_ptr<Records> Records::NewDefaultRecords(std::shared_ptr<RecordBatch> batch)
{
    auto r = std::make_shared<Records>();
    r->RecordsType = defaultRecords;
    r->RBatch = std::move(batch);
    return r;
}

int Records::SetTypeFromFields(bool &empty)
{
    empty = false;
    if (MsgSet == nullptr && RBatch == nullptr)
    {
        empty = true;
        return 0;
    }
    if (MsgSet != nullptr && RBatch != nullptr)
    {
        return -1;
    }
    RecordsType = defaultRecords;
    if (MsgSet != nullptr)
    {
        RecordsType = legacyRecords;
    }

    return 0;
}

int Records::encode(PEncoder &pe)
{
    if (RecordsType == unknownRecords)
    {
        bool empty;
        int err = SetTypeFromFields(empty);
        if (err != 0 || empty)
        {
            return err;
        }
    }

    switch (RecordsType)
    {
    case legacyRecords:
        if (MsgSet == nullptr)
            return 0;
        return MsgSet->encode(pe);
    case defaultRecords:
        if (RBatch == nullptr)
            return 0;
        return RBatch->encode(pe);
    default:
        return -1; // unknown type
    }
}

int Records::SetTypeFromMagic(PDecoder &pd)
{
    int8_t magic;
    int err = magicValue(pd, magic);
    if (err != 0)
    {
        return err;
    }

    RecordsType = defaultRecords;
    if (magic < 2)
    {
        RecordsType = legacyRecords;
    }
    return 0;
}

int Records::decode(PDecoder &pd)
{
    if (RecordsType == unknownRecords)
    {
        int err = SetTypeFromMagic(pd);
        if (err != 0)
        {
            return err;
        }
    }

    switch (RecordsType)
    {
    case legacyRecords:
    {
        MsgSet = std::make_shared<MessageSet>();
        return MsgSet->decode(pd);
    }
    case defaultRecords:
    {
        RBatch = std::make_shared<RecordBatch>();
        return RBatch->decode(pd);
    }
    default:
        return -1;
    }
}

int Records::numRecords(int &numRecords)
{
    numRecords = 0;
    if (RecordsType == unknownRecords)
    {
        bool empty;
        int err = SetTypeFromFields(empty);
        if (err != 0 || empty)
        {
            return err;
        }
    }

    switch (RecordsType)
    {
    case legacyRecords:
        if (MsgSet == nullptr)
            return 0;
        numRecords = static_cast<int>(MsgSet->Messages.size());
        return 0;
    case defaultRecords:
        if (RBatch == nullptr)
            return 0;
        numRecords = RBatch->Records.size();
        return 0;
    default:
        return -1;
    }
}

int Records::isPartial(bool &partial)
{
    partial = false;
    if (RecordsType == unknownRecords)
    {
        bool empty;
        int err = SetTypeFromFields(empty);
        if (err != 0 || empty)
        {
            return err;
        }
    }

    switch (RecordsType)
    {
    case unknownRecords:
        return 0;
    case legacyRecords:
        if (MsgSet == nullptr)
            return 0;
        partial = MsgSet->PartialTrailingMessage;
        return 0;
    case defaultRecords:
        if (RBatch == nullptr)
            return 0;
        partial = RBatch->PartialTrailingRecord;
        return 0;
    default:
        return ErrUnknownRecordsType;
    }
}

int Records::isControl(bool &out)
{
    out = false;
    if (RecordsType == unknownRecords)
    {
        bool empty;
        int err = SetTypeFromFields(empty);
        if (err != 0 || empty)
        {
            return err;
        }
    }

    switch (RecordsType)
    {
    case legacyRecords:
        return 0;
    case defaultRecords:
        if (RBatch == nullptr)
            return 0;
        out = RBatch->Control;
        return 0;
    default:
        return -1;
    }
}

int Records::isOverflow(bool &out)
{
    out = false;
    if (RecordsType == unknownRecords)
    {
        bool empty;
        int err = SetTypeFromFields(empty);
        if (err != 0 || empty)
        {
            return err;
        }
    }
    switch (RecordsType)
    {
    case unknownRecords:
        return 0;
    case legacyRecords:
        if (MsgSet == nullptr)
            return 0;
        out = MsgSet->OverflowMessage;
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
    switch (RecordsType)
    {
    case unknownRecords:
    case legacyRecords:
        return 0;
    case defaultRecords:
        if (RBatch == nullptr)
            return 0;
        offset = RBatch->LastOffset() + 1;
        return 0;
    default:
        offset = 0;
        return -1;
    }
}

int Records::getControlRecord(ControlRecord &out)
{
    if (RBatch == nullptr || RBatch->Records.empty())
    {
        return -1;
    }

    auto &firstRecord = RBatch->Records[0];
    realDecoder keyDecoder;
    keyDecoder.Raw = firstRecord->Key;

    realDecoder valueDecoder;
    valueDecoder.Raw = firstRecord->Value;

    int err = out.decode(keyDecoder, valueDecoder);
    if (err != 0)
    {
        return err;
    }

    return 0;
}
