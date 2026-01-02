#include "control_record.h"

int ControlRecord::decode(PDecoder &key, PDecoder &value)
{
    bool success = true;
    int16_t tempVersion;
    success = key.getInt16(tempVersion);
    if (!success)
    {
        return ErrDecodeError;
    }
    Version = tempVersion;

    int16_t recordType;
    success = key.getInt16(recordType);
    if (!success)
    {
        return ErrDecodeError;
    }

    switch (recordType)
    {
    case 0:
        Type = ControlRecordType::ControlRecordAbort;
        break;
    case 1:
        Type = ControlRecordType::ControlRecordCommit;
        break;
    default:
        Type = ControlRecordType::ControlRecordUnknown;
        break;
    }

    if (Type != ControlRecordType::ControlRecordUnknown)
    {
        success = value.getInt16(tempVersion);
        if (!success)
        {
            return ErrDecodeError;
        }
        Version = tempVersion;

        int32_t tempCoordinatorEpoch;
        success = value.getInt32(tempCoordinatorEpoch);
        if (!success)
        {
            return ErrDecodeError;
        }
        CoordinatorEpoch = tempCoordinatorEpoch;
    }

    return ErrNoError;
}

int ControlRecord::encode(PEncoder &key, PEncoder &value)
{
    value.putInt16(Version);
    value.putInt32(CoordinatorEpoch);
    key.putInt16(Version);

    switch (Type)
    {
    case ControlRecordType::ControlRecordAbort:
        key.putInt16(0);
        break;
    case ControlRecordType::ControlRecordCommit:
        key.putInt16(1);
        break;
    default:
        break;
    }
    return ErrNoError;
}