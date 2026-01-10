#pragma once

#include <memory>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "message_set.h"
#include "record_batch.h"
#include "control_record.h"
#include "real_decoder.h"
#include "version.h"

const int unknownRecords = 0;
const int legacyRecords = 1;
const int defaultRecords = 2;

struct Records
{
    int m_records_type = unknownRecords;
    std::shared_ptr<MessageSet> m_msg_set;
    std::shared_ptr<RecordBatch> m_record_batch;

    Records();
    static std::shared_ptr<Records> NewLegacyRecords(std::shared_ptr<MessageSet> msgSet);
    static std::shared_ptr<Records> NewDefaultRecords(std::shared_ptr<RecordBatch> batch);

    int SetTypeFromFields(bool &empty);
    int SetTypeFromMagic(PDecoder &pd);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd);
    int numRecords(int &);
    int isPartial(bool &);
    int isControl(bool &);
    int isOverflow(bool &);
    int nextOffset(int64_t &offset);
    int getControlRecord(ControlRecord &);
};
