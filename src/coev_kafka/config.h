#pragma once

#include <memory>
#include <functional>
#include <vector>
#include <string>
#include <regex>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "../utils/compress/coev_compress.h"
#include "../utils/hash/fnv.h"
#include "isolation_level.h"
#include "version.h"
#include "metrics.h"

struct SCRAMClient;
struct AccessTokenProvider;
struct GSSAPIConfig;
struct ProxyDialer;
struct ProducerInterceptor;
struct Partitioner;
struct BalanceStrategy;
struct ConsumerInterceptor;

inline constexpr const char *SASLTypeOAuth = "OAUTHBEARER";
inline constexpr const char *SASLTypePlaintext = "PLAIN";
inline constexpr const char *SASLTypeSCRAMSHA256 = "SCRAM-SAH-256";
inline constexpr const char *SASLTypeSCRAMSHA512 = "SCRAM-SHA-512";
inline constexpr const char *SASLTypeGSSAPI = "GSSAPI";
inline constexpr const char *SASLExtKeyAuth = "auth";
inline constexpr int16_t SASLHandshakeV0 = 0;
inline constexpr int16_t SASLHandshakeV1 = 1;
inline constexpr int64_t OffsetNewest = -1;
inline constexpr int64_t OffsetOldest = -2;
inline constexpr int64_t AllPartitions = 0;
inline constexpr int64_t WritablePartitions_ = 1;
inline constexpr int64_t MaxPartitionIndex = 2;

using SASLMechanism = std::string;
using PartitionerConstructor = std::function<std::shared_ptr<Partitioner>(const std::string &topic)>;

enum RequiredAcks : int16_t
{
    NoResponse = 0,
    WaitForLocal = 1,
    WaitForAll = -1
};

struct admin_config
{
    struct retry_config
    {
        int Max = 5;
        std::chrono::milliseconds Backoff = std::chrono::milliseconds(100);
    } Retry;
    std::chrono::milliseconds Timeout = std::chrono::milliseconds(3000);
};

struct net_config
{
    int MaxOpenRequests = 5;
    std::chrono::milliseconds DialTimeout = std::chrono::milliseconds(30000);
    std::chrono::milliseconds ReadTimeout = std::chrono::milliseconds(30000);
    std::chrono::milliseconds WriteTimeout = std::chrono::milliseconds(30000);
    bool ResolveCanonicalBootstrapServers = false;

    struct tls_config
    {
        bool Enable = false;
        std::shared_ptr<tls_config> Config;
    } TLS;

    struct sasl_config
    {
        bool Enable = false;
        SASLMechanism Mechanism;
        int16_t Version = SASLHandshakeV1;
        bool Handshake = true;
        std::string AuthIdentity;
        std::string User;
        std::string Password;
        std::string ScramAuthzID;
        std::function<std::shared_ptr<SCRAMClient>()> ScramClientGeneratorFunc;
        std::shared_ptr<AccessTokenProvider> TokenProvider_;
        std::shared_ptr<GSSAPIConfig> GSSAPI;
    } SASL;

    std::chrono::milliseconds KeepAlive = std::chrono::milliseconds(0);
    sockaddr_in LocalAddr;
    struct proxy_config
    {
        bool Enable = false;
        std::shared_ptr<ProxyDialer> Dialer;
    } Proxy;
};

struct metadata_config
{
    struct retry_config
    {
        int Max = 3;
        std::chrono::milliseconds Backoff = std::chrono::milliseconds(250);
        std::function<std::chrono::milliseconds(int, int)> BackoffFunc;
    } Retry;
    std::chrono::milliseconds RefreshFrequency = std::chrono::minutes(10);
    bool Full = true;
    std::chrono::milliseconds Timeout = std::chrono::milliseconds(0);
    bool AllowAutoTopicCreation = true;
    bool SingleLight = true;
};

struct producer_transaction_config
{
    std::string ID;
    std::chrono::milliseconds Timeout = std::chrono::minutes(1);

    struct retry_config
    {
        int Max = 50;
        std::chrono::milliseconds Backoff = std::chrono::milliseconds(100);
        std::function<std::chrono::milliseconds(int, int)> BackoffFunc;
    } Retry;
};

struct producer_flush_config
{
    int Bytes = 0;
    int Messages = 0;
    std::chrono::milliseconds Frequency = std::chrono::milliseconds(0);
    int MaxMessages = 0;
};

struct producer_retry_config
{
    int Max = 3;
    std::chrono::milliseconds Backoff = std::chrono::milliseconds(100);
    std::function<std::chrono::milliseconds(int, int)> BackoffFunc;
    int MaxBufferLength = 0;
    int64_t MaxBufferBytes = 0;
};

struct producer_return_config
{
    bool Successes = false;
    bool Errors = true;
};

struct producer_config
{
    int MaxMessageBytes = 1024 * 1024;
    RequiredAcks Acks = RequiredAcks::WaitForLocal;
    std::chrono::milliseconds Timeout = std::chrono::milliseconds(10000);
    CompressionCodec Compression;
    int CompressionLevel = 0;
    PartitionerConstructor Partitioner;
    bool Idempotent = false;
    producer_transaction_config Transaction;
    producer_flush_config Flush;
    producer_retry_config Retry;
    producer_return_config Return;
    std::vector<std::shared_ptr<ProducerInterceptor>> Interceptors;
};

struct consumer_group_session_config
{
    std::chrono::milliseconds Timeout = std::chrono::milliseconds(10000);
};

struct consumer_group_heartbeat_config
{
    std::chrono::milliseconds Interval = std::chrono::milliseconds(3000);
};

struct consumer_group_rebalance_retry_config
{
    int Max = 4;
    std::chrono::milliseconds Backoff = std::chrono::milliseconds(2000);
};

struct consumer_group_rebalance_config
{
    std::shared_ptr<BalanceStrategy> Strategy;
    std::vector<std::shared_ptr<BalanceStrategy>> GroupStrategies;
    std::chrono::milliseconds Timeout = std::chrono::milliseconds(60000);
    consumer_group_rebalance_retry_config Retry;
};

struct consumer_group_member_config
{
    std::string UserData;
};

struct consumer_group_config
{
    consumer_group_session_config Session;
    consumer_group_heartbeat_config Heartbeat;
    consumer_group_rebalance_config Rebalance;
    consumer_group_member_config Member;
    std::string InstanceID;
    bool ResetInvalidOffsets = true;
};

struct consumer_retry_config
{
    std::chrono::milliseconds Backoff = std::chrono::milliseconds(2000);
    std::function<std::chrono::milliseconds(int)> BackoffFunc;
};

struct consumer_fetch_config
{
    int32_t Min = 1;
    int32_t DefaultVal = 1024 * 1024;
    int32_t Max = 0;
};

struct consumer_offsets_auto_commit_config
{
    bool Enable = true;
    std::chrono::milliseconds Interval = std::chrono::milliseconds(1000);
};

struct consumer_offsets_retry_config
{
    int Max = 3;
};

struct consumer_offsets_config
{
    std::chrono::milliseconds CommitInterval = std::chrono::milliseconds(0);
    consumer_offsets_auto_commit_config AutoCommit;
    int64_t Initial = OffsetNewest;
    std::chrono::milliseconds Retention = std::chrono::milliseconds(0);
    consumer_offsets_retry_config Retry;
};

struct consumer_return_config
{
    bool Errors = false;
};

struct consumer_config
{
    consumer_group_config Group;
    consumer_retry_config Retry;
    consumer_fetch_config Fetch;
    std::chrono::milliseconds MaxWaitTime = std::chrono::milliseconds(500);
    std::chrono::milliseconds MaxProcessingTime = std::chrono::milliseconds(100);
    consumer_return_config Return;
    consumer_offsets_config Offsets;
    IsolationLevel IsolationLevel_ = IsolationLevel::ReadUncommitted;
    std::vector<std::shared_ptr<ConsumerInterceptor>> Interceptors;
};

struct Config
{
    admin_config Admin;
    net_config Net;
    metadata_config Metadata;
    producer_config Producer;
    consumer_config Consumer;
    std::string ClientID = "coev";
    std::string RackId;
    int ChannelBufferSize = 256;
    bool ApiVersionsRequest = true;
    KafkaVersion Version;
    std::shared_ptr<metrics::Registry> MetricRegistry;

    Config();
    bool Validate();
    std::shared_ptr<Config> Clone() const;
    std::shared_ptr<ProxyDialer> GetDialer() const;
};

bool ValidateGroupInstanceId(const std::string &id);
