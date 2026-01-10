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
    m_version = tempVersion;

    int16_t recordType;
    success = key.getInt16(recordType);
    if (!success)
    {
        return ErrDecodeError;
    }

    switch (recordType)
    {
    case 0:
        m_type = ControlRecordType::ControlRecordAbort;
        break;
    case 1:
        m_type = ControlRecordType::ControlRecordCommit;
        break;
    default:
        m_type = ControlRecordType::ControlRecordUnknown;
        break;
    }

    if (m_type != ControlRecordType::ControlRecordUnknown)
    {
        success = value.getInt16(tempVersion);
        if (!success)
        {
            return ErrDecodeError;
        }
        m_version = tempVersion;

        int32_t tempCoordinatorEpoch;
        success = value.getInt32(tempCoordinatorEpoch);
        if (!success)
        {
            return ErrDecodeError;
        }
        m_coordinator_epoch = tempCoordinatorEpoch;
    }

    return ErrNoError;
}

int ControlRecord::encode(PEncoder &key, PEncoder &value)
{
    value.putInt16(m_version);
    value.putInt32(m_coordinator_epoch);
    key.putInt16(m_version);

    switch (m_type)
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