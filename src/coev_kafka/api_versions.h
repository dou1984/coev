#pragma once

#include <map>
#include <cstdint>
#include "request.h"

struct ApiVersionRange
{
    int16_t m_min_version = 0;
    int16_t m_max_version = 0;

    ApiVersionRange(int16_t min = 0, int16_t max = 0) : m_min_version(min), m_max_version(max) {}
};

using ApiVersionMap = std::map<int16_t, ApiVersionRange>;

inline constexpr int16_t apiKeyProduce = 0;
inline constexpr int16_t apiKeyFetch = 1;
inline constexpr int16_t apiKeyListOffsets = 2;
inline constexpr int16_t apiKeyMetadata = 3;
inline constexpr int16_t apiKeyLeaderAndIsr = 4;
inline constexpr int16_t apiKeyStopReplica = 5;
inline constexpr int16_t apiKeyUpdateMetadata = 6;
inline constexpr int16_t apiKeyControlledShutdown = 7;
inline constexpr int16_t apiKeyOffsetCommit = 8;
inline constexpr int16_t apiKeyOffsetFetch = 9;
inline constexpr int16_t apiKeyFindCoordinator = 10;
inline constexpr int16_t apiKeyJoinGroup = 11;
inline constexpr int16_t apiKeyHeartbeat = 12;
inline constexpr int16_t apiKeyLeaveGroup = 13;
inline constexpr int16_t apiKeySyncGroup = 14;
inline constexpr int16_t apiKeyDescribeGroups = 15;
inline constexpr int16_t apiKeyListGroups = 16;
inline constexpr int16_t apiKeySaslHandshake = 17;
inline constexpr int16_t apiKeyApiVersions = 18;
inline constexpr int16_t apiKeyCreateTopics = 19;
inline constexpr int16_t apiKeyDeleteTopics = 20;
inline constexpr int16_t apiKeyDeleteRecords = 21;
inline constexpr int16_t apiKeyInitProducerId = 22;
inline constexpr int16_t apiKeyOffsetForLeaderEpoch = 23;
inline constexpr int16_t apiKeyAddPartitionsToTxn = 24;
inline constexpr int16_t apiKeyAddOffsetsToTxn = 25;
inline constexpr int16_t apiKeyEndTxn = 26;
inline constexpr int16_t apiKeyWriteTxnMarkers = 27;
inline constexpr int16_t apiKeyTxnOffsetCommit = 28;
inline constexpr int16_t apiKeyDescribeAcls = 29;
inline constexpr int16_t apiKeyCreateAcls = 30;
inline constexpr int16_t apiKeyDeleteAcls = 31;
inline constexpr int16_t apiKeyDescribeConfigs = 32;
inline constexpr int16_t apiKeyAlterConfigs = 33;
inline constexpr int16_t apiKeyAlterReplicaLogDirs = 34;
inline constexpr int16_t apiKeyDescribeLogDirs = 35;
inline constexpr int16_t apiKeySASLAuth = 36;
inline constexpr int16_t apiKeyCreatePartitions = 37;
inline constexpr int16_t apiKeyCreateDelegationToken = 38;
inline constexpr int16_t apiKeyRenewDelegationToken = 39;
inline constexpr int16_t apiKeyExpireDelegationToken = 40;
inline constexpr int16_t apiKeyDescribeDelegationToken = 41;
inline constexpr int16_t apiKeyDeleteGroups = 42;
inline constexpr int16_t apiKeyElectLeaders = 43;
inline constexpr int16_t apiKeyIncrementalAlterConfigs = 44;
inline constexpr int16_t apiKeyAlterPartitionReassignments = 45;
inline constexpr int16_t apiKeyListPartitionReassignments = 46;
inline constexpr int16_t apiKeyOffsetDelete = 47;
inline constexpr int16_t apiKeyDescribeClientQuotas = 48;
inline constexpr int16_t apiKeyAlterClientQuotas = 49;
inline constexpr int16_t apiKeyDescribeUserScramCredentials = 50;
inline constexpr int16_t apiKeyAlterUserScramCredentials = 51;

template <class T>
void RestrictApiVersion(const T &pb, const ApiVersionMap &brokerVersions)
{

    int16_t key = pb.key();
    int16_t clientMax = pb.version();

    auto it = brokerVersions.find(key);
    if (it != brokerVersions.end())
    {
        const ApiVersionRange &range = it->second;
        int16_t selected = std::min(clientMax, std::max(range.m_min_version, std::min(clientMax, range.m_max_version)));

        auto _pb = const_cast<protocol_body *>(static_cast<const protocol_body *>(&pb));
        _pb->set_version(selected);
    }
}