#include "control_record.h"

int ControlRecord::decode(packet_decoder &key, packet_decoder &value)
{
    int success = 0;
    int16_t tempVersion;

    success = key.getInt16(tempVersion);
    if (success != 0)
    {
        return ErrDecodeError;
    }
    m_version = tempVersion;

    int16_t recordType;
    success = key.getInt16(recordType);
    if (success != 0)
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

    if (m_type != ControlRecordType::ControlRecordUnknown && value.remaining() > 0)
    {
        success = value.getInt16(tempVersion);
        if (success != 0)
        {
            return ErrDecodeError;
        }

        if (tempVersion != m_version)
        {
            return ErrDecodeError;
        }

        int32_t tempCoordinatorEpoch;
        success = value.getInt32(tempCoordinatorEpoch);
        if (success != 0)
        {
            return ErrDecodeError;
        }
        m_coordinator_epoch = tempCoordinatorEpoch;
    }

    return ErrNoError;
}

int ControlRecord::encode(packet_encoder &key, packet_encoder &value)
{
    key.putInt16(m_version);

    switch (m_type)
    {
    case ControlRecordType::ControlRecordAbort:
        key.putInt16(0);
        break;
    case ControlRecordType::ControlRecordCommit:
        key.putInt16(1);
        break;
    case ControlRecordType::ControlRecordUnknown:
        key.putInt16(128); // UNKNOWN = -1
        break;
    default:
        key.putInt16(128); // UNKNOWN = -1
        break;
    }

    if (m_type != ControlRecordType::ControlRecordUnknown)
    {
        value.putInt16(m_version);
        value.putInt32(m_coordinator_epoch);
    }

    return ErrNoError;
}