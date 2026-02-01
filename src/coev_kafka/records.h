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

const int UnknownRecords = 0;
const int LegacyRecords = 1;
const int DefaultRecords = 2;

struct Records : std::enable_shared_from_this<Records>
{
    mutable int m_records_type = UnknownRecords;
    mutable std::shared_ptr<MessageSet> m_message_set;
    std::shared_ptr<RecordBatch> m_record_batch;

    Records();
    Records(std::shared_ptr<MessageSet> msgSet);
    Records(std::shared_ptr<RecordBatch> batch);

    int set_type_from_fields(bool &empty) const;
    int set_type_from_magic(packet_decoder &pd);
    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd);
    int num_records(int &) const;
    int is_partial(bool &) const;
    int is_control(bool &) const;
    int is_overflow(bool &);
    int next_offset(int64_t &offset);
    int get_control_record(ControlRecord &) const;
};
