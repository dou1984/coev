#include "partition_offset_manager.h"

PartitionOffsetManager::PartitionOffsetManager(
    std::shared_ptr<OffsetManager> parent, const std::string &topic,
    int32_t partition, int32_t leaderEpoch, size_t channelBufferSize, int64_t offset, const std::string &metadata)
    : Parent(parent), Topic(topic), Partition(partition), LeaderEpoch(leaderEpoch), Offset(offset), Metadata(metadata), dirty(false), done(false)
{
}

std::pair<int64_t, std::string> PartitionOffsetManager::NextOffset()
{
    std::unique_lock<std::mutex> l(Lock);
    if (Offset >= 0)
    {
        return std::make_pair(Offset, Metadata);
    }
    return std::make_pair(Parent->conf->Consumer.Offsets.Initial, "");
}
coev::co_channel<std::shared_ptr<ConsumerError>> &PartitionOffsetManager::Errors()
{
    return errors;
}
void PartitionOffsetManager::MarkOffset(int64_t offset, const std::string &metadata)
{
    std::unique_lock<std::mutex> l(Lock);
    if (offset > this->Offset)
    {
        this->Offset = offset;
        this->Metadata = metadata;
        this->dirty = true;
    }
}

void PartitionOffsetManager::ResetOffset(int64_t offset, const std::string &metadata)
{
    std::unique_lock<std::mutex> l(Lock);
    if (offset <= this->Offset)
    {
        this->Offset = offset;
        this->Metadata = metadata;
        this->dirty = true;
    }
}

void PartitionOffsetManager::UpdateCommitted(int64_t offset, const std::string &metadata)
{
    std::unique_lock<std::mutex> l(Lock);
    if (this->Offset == offset && this->Metadata == metadata)
    {
        this->dirty = false;
    }
}

void PartitionOffsetManager::AsyncClose()
{
    std::unique_lock<std::mutex> l(Lock);
    done = true;
}

void PartitionOffsetManager::Close()
{
    AsyncClose();
}

void PartitionOffsetManager::HandleError(KError err)
{
    auto cErr = std::make_shared<ConsumerError>();
    cErr->Topic = Topic;
    cErr->Partition = Partition;
    cErr->Err = err;

    if (Parent->conf->Consumer.Return.Errors)
    {
        errors.set(cErr);
    }
}

void PartitionOffsetManager::Release()
{
}