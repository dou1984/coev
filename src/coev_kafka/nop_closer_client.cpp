#include "nop_closer_client.h"

NopCloserClient::NopCloserClient(std::shared_ptr<IClient> client) : client_(client)
{
}
int NopCloserClient::Close()
{
    return 0;
}

std::shared_ptr<Config> NopCloserClient::GetConfig()
{
    return client_->GetConfig();
}
coev::awaitable<int> NopCloserClient::Controller(std::shared_ptr<Broker> &broker)
{
    return client_->Controller(broker);
}
coev::awaitable<int> NopCloserClient::RefreshController(std::shared_ptr<Broker> &broker)
{
    return client_->RefreshController(broker);
}
std::vector<std::shared_ptr<Broker>> NopCloserClient::Brokers()
{
    return client_->Brokers();
}
coev::awaitable<int> NopCloserClient::GetBroker(int32_t brokerID, std::shared_ptr<Broker> &broker)
{
    return client_->GetBroker(brokerID, broker);
}
int NopCloserClient::Topics(std::vector<std::string> &topics)
{
    return client_->Topics(topics);
}
coev::awaitable<int> NopCloserClient::Partitions(const std::string &topic, std::vector<int32_t> &partitions)
{
    return client_->Partitions(topic, partitions);
}
coev::awaitable<int> NopCloserClient::WritablePartitions(const std::string &topic, std::vector<int32_t> &partitions)
{
    return client_->WritablePartitions(topic, partitions);
}
coev::awaitable<int> NopCloserClient::Leader(const std::string &topic, int32_t partitionID, std::shared_ptr<Broker> &leader)
{
    return client_->Leader(topic, partitionID, leader);
}
coev::awaitable<int> NopCloserClient::LeaderAndEpoch(const std::string &topic, int32_t partitionID, std::shared_ptr<Broker> &leader, int32_t &epoch)
{
    return client_->LeaderAndEpoch(topic, partitionID, leader, epoch);
}
coev::awaitable<int> NopCloserClient::Replicas(const std::string &topic, int32_t partitionID, std::vector<int32_t> &replicas)
{
    return client_->Replicas(topic, partitionID, replicas);
}
coev::awaitable<int> NopCloserClient::InSyncReplicas(const std::string &topic, int32_t partitionID, std::vector<int32_t> &isr)
{
    return client_->InSyncReplicas(topic, partitionID, isr);
}
coev::awaitable<int> NopCloserClient::OfflineReplicas(const std::string &topic, int32_t partitionID, std::vector<int32_t> &offline)
{
    return client_->OfflineReplicas(topic, partitionID, offline);
}
coev::awaitable<int> NopCloserClient::RefreshBrokers(const std::vector<std::string> &addrs)
{
    return client_->RefreshBrokers(addrs);
}
coev::awaitable<int> NopCloserClient::RefreshMetadata(const std::vector<std::string> &topics)
{
    return client_->RefreshMetadata(topics);
}
coev::awaitable<int> NopCloserClient::GetOffset(const std::string &topic, int32_t partitionID, int64_t time, int64_t &offset)
{
    return client_->GetOffset(topic, partitionID, time, offset);
}
coev::awaitable<int> NopCloserClient::GetCoordinator(const std::string &consumerGroup, std::shared_ptr<Broker> &coordinator)
{
    return client_->GetCoordinator(consumerGroup, coordinator);
}
coev::awaitable<int> NopCloserClient::RefreshCoordinator(const std::string &consumerGroup)
{
    return client_->RefreshCoordinator(consumerGroup);
}
coev::awaitable<int> NopCloserClient::TransactionCoordinator(const std::string &transactionID, std::shared_ptr<Broker> &coordinator)
{
    return client_->TransactionCoordinator(transactionID, coordinator);
}
coev::awaitable<int> NopCloserClient::RefreshTransactionCoordinator(const std::string &transactionID)
{
    return client_->RefreshTransactionCoordinator(transactionID);
}
coev::awaitable<int> NopCloserClient::InitProducerID(std::shared_ptr<InitProducerIDResponse> &response)
{
    return client_->InitProducerID(response);
}
coev::awaitable<int> NopCloserClient::LeastLoadedBroker(std::shared_ptr<Broker> &broker)
{
    return client_->LeastLoadedBroker(broker);
}
bool NopCloserClient::PartitionNotReadable(const std::string &topic, int32_t partition)
{
    return client_->PartitionNotReadable(topic, partition);
}
bool NopCloserClient::Closed()
{
    return client_->Closed();
}