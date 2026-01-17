#include <regex>
#include <iostream>
#include <algorithm>
#include <limits>
#include <utils/hash/fnv.h>
#include "config.h"
#include "version.h"
#include "client.h"
#include "balance_strategy.h"
#include "undefined.h"
#include "partitioner.h"

const int MAX_GROUP_INSTANCE_ID_LENGTH = 249;
const std::regex GROUP_INSTANCE_ID_REGEXP(R"(^[0-9a-zA-Z\._\-]+$)");

Config::Config()
{
    Admin.Retry.Max = 5;
    Admin.Retry.Backoff = std::chrono::milliseconds(100);
    Admin.Timeout = std::chrono::milliseconds(3000);

    Net.MaxOpenRequests = 5;
    Net.DialTimeout = std::chrono::milliseconds(30000);
    Net.ReadTimeout = std::chrono::milliseconds(30000);
    Net.WriteTimeout = std::chrono::milliseconds(30000);
    Net.SASL.Handshake = true;
    Net.SASL.Version = SASLHandshakeV1;

    Metadata.Retry.Max = 3;
    Metadata.Retry.Backoff = std::chrono::milliseconds(250);
    Metadata.RefreshFrequency = std::chrono::minutes(10);
    Metadata.Full = true;
    Metadata.AllowAutoTopicCreation = true;
    Metadata.SingleLight = true;

    Producer.MaxMessageBytes = 1024 * 1024;
    Producer.Acks = RequiredAcks::WaitForLocal;
    Producer.Timeout = std::chrono::milliseconds(10000);
    Producer.Partitioner = NewCustomHashPartitioner(coev::fnv::New32a);
    Producer.Retry.Max = 3;
    Producer.Retry.Backoff = std::chrono::milliseconds(100);
    Producer.Return.Errors = true;
    Producer.CompressionLevel = CompressionLevelDefault;

    Producer.Transaction.Timeout = std::chrono::minutes(1);
    Producer.Transaction.Retry.Max = 50;
    Producer.Transaction.Retry.Backoff = std::chrono::milliseconds(100);

    Consumer.Fetch.Min = 1;
    Consumer.Fetch.DefaultVal = 1024 * 1024;
    Consumer.Retry.Backoff = std::chrono::milliseconds(2000);
    Consumer.MaxWaitTime = std::chrono::milliseconds(500);
    Consumer.MaxProcessingTime = std::chrono::milliseconds(100);
    Consumer.Return.Errors = false;
    Consumer.Offsets.AutoCommit.Enable = true;
    Consumer.Offsets.AutoCommit.Interval = std::chrono::milliseconds(1000);
    Consumer.Offsets.Initial = OffsetNewest;
    Consumer.Offsets.Retry.Max = 3;

    Consumer.Group.Session.Timeout = std::chrono::milliseconds(10000);
    Consumer.Group.Heartbeat.Interval = std::chrono::milliseconds(3000);
    Consumer.Group.Rebalance.GroupStrategies.emplace_back(NewBalanceStrategyRange());
    Consumer.Group.Rebalance.Timeout = std::chrono::milliseconds(60000);
    Consumer.Group.Rebalance.Retry.Max = 4;
    Consumer.Group.Rebalance.Retry.Backoff = std::chrono::milliseconds(2000);
    Consumer.Group.ResetInvalidOffsets = true;

    ClientID = "coev";
    ChannelBufferSize = 256;
    ApiVersionsRequest = true;
    Version = DefaultVersion;
}

std::shared_ptr<Config> Config::Clone() const
{
    return std::make_shared<Config>(*this);
}

bool Config::Validate()
{

    if (Net.MaxOpenRequests <= 0)
    {
        std::cerr << "Net.MaxOpenRequests must be > 0" << std::endl;
        return false;
    }
    if (Net.DialTimeout <= std::chrono::milliseconds(0))
    {
        std::cerr << "Net.DialTimeout must be > 0" << std::endl;
        return false;
    }
    if (Net.ReadTimeout <= std::chrono::milliseconds(0))
    {
        std::cerr << "Net.ReadTimeout must be > 0" << std::endl;
        return false;
    }
    if (Net.WriteTimeout <= std::chrono::milliseconds(0))
    {
        std::cerr << "Net.WriteTimeout must be > 0" << std::endl;
        return false;
    }

    if (Net.SASL.Enable)
    {
        if (Net.SASL.Mechanism == SASLMechanism{})
        {
            Net.SASL.Mechanism = SASLTypePlaintext;
        }
    }

    if (Admin.Timeout <= std::chrono::milliseconds(0))
    {
        std::cerr << "Admin.Timeout must be > 0" << std::endl;
        return false;
    }

    if (Metadata.Retry.Max < 0)
    {
        std::cerr << "Metadata.Retry.Max must be >= 0" << std::endl;
        return false;
    }
    if (Metadata.Retry.Backoff < std::chrono::milliseconds(0))
    {
        std::cerr << "Metadata.Retry.Backoff must be >= 0" << std::endl;
        return false;
    }
    if (Metadata.RefreshFrequency < std::chrono::milliseconds(0))
    {
        std::cerr << "Metadata.RefreshFrequency must be >= 0" << std::endl;
        return false;
    }

    if (Producer.MaxMessageBytes <= 0)
    {
        std::cerr << "Producer.MaxMessageBytes must be > 0" << std::endl;
        return false;
    }
    if (static_cast<int>(Producer.Acks) < -1)
    {
        std::cerr << "Producer.RequiredAcks must be >= -1" << std::endl;
        return false;
    }
    if (Producer.Timeout <= std::chrono::milliseconds(0))
    {
        std::cerr << "Producer.Timeout must be > 0" << std::endl;
        return false;
    }
    if (!Producer.Partitioner)
    {
        std::cerr << "Producer.Partitioner must not be nil" << std::endl;
        return false;
    }
    if (Producer.Flush.Bytes < 0)
    {
        std::cerr << "Producer.Flush.Bytes must be >= 0" << std::endl;
        return false;
    }
    if (Producer.Flush.Messages < 0)
    {
        std::cerr << "Producer.Flush.Messages must be >= 0" << std::endl;
        return false;
    }
    if (Producer.Flush.Frequency < std::chrono::milliseconds(0))
    {
        std::cerr << "Producer.Flush.Frequency must be >= 0" << std::endl;
        return false;
    }
    if (Producer.Flush.MaxMessages < 0)
    {
        std::cerr << "Producer.Flush.MaxMessages must be >= 0" << std::endl;
        return false;
    }
    if (Producer.Flush.MaxMessages > 0 && Producer.Flush.MaxMessages < Producer.Flush.Messages)
    {
        std::cerr << "Producer.Flush.MaxMessages must be >= Producer.Flush.Messages when set" << std::endl;
        return false;
    }
    if (Producer.Retry.Max < 0)
    {
        std::cerr << "Producer.Retry.Max must be >= 0" << std::endl;
        return false;
    }
    if (Producer.Retry.Backoff < std::chrono::milliseconds(0))
    {
        std::cerr << "Producer.Retry.Backoff must be >= 0" << std::endl;
        return false;
    }

    // Validate Consumer values
    if (Consumer.Fetch.Min <= 0)
    {
        std::cerr << "Consumer.Fetch.Min must be > 0" << std::endl;
        return false;
    }
    if (Consumer.Fetch.DefaultVal <= 0)
    {
        std::cerr << "Consumer.Fetch.Default must be > 0" << std::endl;
        return false;
    }
    if (Consumer.Fetch.Max < 0)
    {
        std::cerr << "Consumer.Fetch.Max must be >= 0" << std::endl;
        return false;
    }
    if (Consumer.MaxWaitTime < std::chrono::milliseconds(1))
    {
        std::cerr << "Consumer.MaxWaitTime must be >= 1ms" << std::endl;
        return false;
    }
    if (Consumer.MaxProcessingTime <= std::chrono::milliseconds(0))
    {
        std::cerr << "Consumer.MaxProcessingTime must be > 0" << std::endl;
        return false;
    }
    if (Consumer.Retry.Backoff < std::chrono::milliseconds(0))
    {
        std::cerr << "Consumer.Retry.Backoff must be >= 0" << std::endl;
        return false;
    }
    if (Consumer.Offsets.AutoCommit.Interval <= std::chrono::milliseconds(0))
    {
        std::cerr << "Consumer.Offsets.AutoCommit.Interval must be > 0" << std::endl;
        return false;
    }
    if (Consumer.Offsets.Initial != OffsetOldest && Consumer.Offsets.Initial != OffsetNewest)
    {
        std::cerr << "Consumer.Offsets.Initial must be OffsetOldest or OffsetNewest" << std::endl;
        return false;
    }
    if (Consumer.Offsets.Retry.Max < 0)
    {
        std::cerr << "Consumer.Offsets.Retry.Max must be >= 0" << std::endl;
        return false;
    }
    if (Consumer.Group.Session.Timeout <= std::chrono::milliseconds(2))
    {
        std::cerr << "Consumer.Group.Session.Timeout must be >= 2ms" << std::endl;
        return false;
    }
    if (Consumer.Group.Heartbeat.Interval < std::chrono::milliseconds(1))
    {
        std::cerr << "Consumer.Group.Heartbeat.Interval must be >= 1ms" << std::endl;
        return false;
    }
    if (Consumer.Group.Heartbeat.Interval >= Consumer.Group.Session.Timeout)
    {
        std::cerr << "Consumer.Group.Heartbeat.Interval must be < Consumer.Group.Session.Timeout" << std::endl;
        return false;
    }
    if (!Consumer.Group.Rebalance.Strategy && Consumer.Group.Rebalance.GroupStrategies.empty())
    {
        std::cerr << "Consumer.Group.Rebalance.GroupStrategies or Consumer.Group.Rebalance.Strategy must not be empty" << std::endl;
        return false;
    }
    if (Consumer.Group.Rebalance.Timeout <= std::chrono::milliseconds(1))
    {
        std::cerr << "Consumer.Group.Rebalance.Timeout must be >= 1ms" << std::endl;
        return false;
    }
    if (Consumer.Group.Rebalance.Retry.Max < 0)
    {
        std::cerr << "Consumer.Group.Rebalance.Retry.Max must be >= 0" << std::endl;
        return false;
    }
    if (Consumer.Group.Rebalance.Retry.Backoff < std::chrono::milliseconds(0))
    {
        std::cerr << "Consumer.Group.Rebalance.Retry.Backoff must be >= 0" << std::endl;
        return false;
    }

    // Validate misc shared values
    if (ChannelBufferSize < 0)
    {
        std::cerr << "ChannelBufferSize must be >= 0" << std::endl;
        return false;
    }

    std::regex validClientID{"^[A-Za-z0-9._-]+$"};
    if (Version.IsAtLeast(V1_0_0_0) && !regex_match(ClientID, validClientID))
    {
        std::cerr << "ClientId must be a valid client ID" << std::endl;
        return false;
    }

    return true;
}

std::shared_ptr<ProxyDialer> Config::GetDialer() const
{
    if (Net.Proxy.Enable)
    {
        std::cout << "using proxy" << std::endl;
        return Net.Proxy.Dialer;
    }
    else
    {
        return std::make_shared<ProxyDialer>(Net.DialTimeout, Net.KeepAlive, Net.LocalAddr);
    }
}

bool ValidateGroupInstanceId(const std::string &id)
{
    if (id.empty())
    {
        std::cerr << "Group instance id must be non-empty string" << std::endl;
        return false;
    }
    if (id == "." || id == "..")
    {
        std::cerr << "Group instance id cannot be \".\" or \"..\"" << std::endl;
        return false;
    }
    if (id.length() > MAX_GROUP_INSTANCE_ID_LENGTH)
    {
        std::cerr << "Group instance id cannot be longer than " << MAX_GROUP_INSTANCE_ID_LENGTH << " characters: " << id << std::endl;
        return false;
    }
    if (!std::regex_match(id, GROUP_INSTANCE_ID_REGEXP))
    {
        std::cerr << "Group instance id " << id << " is illegal, it contains a character other than, '.', '_' and '-'" << std::endl;
        return false;
    }
    return true;
}