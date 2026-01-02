#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"
#include "errors.h"
#include "protocol_body.h"

struct DescribeLogDirsResponsePartition
{
    int64_t Size;
    int64_t OffsetLag;
    int32_t PartitionID;
    bool IsTemporary;

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct DescribeLogDirsResponseTopic : VDecoder, VEncoder
{
    std::string Topic;
    std::vector<DescribeLogDirsResponsePartition> Partitions;

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct DescribeLogDirsResponseDirMetadata : VDecoder, VEncoder
{
    KError ErrorCode;
    std::string Path;
    std::vector<DescribeLogDirsResponseTopic> Topics;
    int64_t TotalBytes;
    int64_t UsableBytes;

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct DescribeLogDirsResponse : protocolBody
{

    std::chrono::milliseconds ThrottleTime;
    int16_t Version;
    std::vector<DescribeLogDirsResponseDirMetadata> LogDirs;
    KError ErrorCode;

    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    bool isFlexible() const;
    static bool isFlexibleVersion(int16_t version);
    KafkaVersion requiredVersion() const;
    std::chrono::milliseconds throttleTime() const;
};
