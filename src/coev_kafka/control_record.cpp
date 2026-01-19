#include "control_record.h"

int ControlRecord::decode(packetDecoder &key, packetDecoder &value)
{
    int success = 0;
    int16_t tempVersion;

    // Decode key first
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

    // Only decode value if it's not an unknown record type and there's data available
    if (m_type != ControlRecordType::ControlRecordUnknown && value.remaining() > 0)
    {
        // Value has its own version field, but it should match the key's version
        success = value.getInt16(tempVersion);
        if (success != 0)
        {
            return ErrDecodeError;
        }

        // Validate that versions match
        if (tempVersion != m_version)
        {
            return ErrDecodeError;
        }

        // Decode coordinator epoch
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

int ControlRecord::encode(packetEncoder &key, packetEncoder &value)
{
    // Encode key first: version + type
    key.putInt16(m_version);

    // Encode record type
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

    // Only encode value if it's not an unknown record type
    if (m_type != ControlRecordType::ControlRecordUnknown)
    {
        // Encode value: version + coordinator epoch
        value.putInt16(m_version);
        value.putInt32(m_coordinator_epoch);
    }

    return ErrNoError;
}