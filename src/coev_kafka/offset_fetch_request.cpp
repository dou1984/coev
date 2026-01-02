#include "version.h"
#include "offset_fetch_request.h"
#include <stdexcept>
#include "api_versions.h"

OffsetFetchRequest::OffsetFetchRequest()
    : Version(0), RequireStable(false)
{
}

std::shared_ptr<OffsetFetchRequest> OffsetFetchRequest::NewOffsetFetchRequest(const KafkaVersion &version, const std::string &group, const std::map<std::string, std::vector<int32_t>> &partitions)
{
    auto request = std::make_shared<OffsetFetchRequest>();
    request->ConsumerGroup = group;
    request->partitions = partitions;

    if (version.IsAtLeast(V2_5_0_0))
    {
        request->Version = 7;
    }
    else if (version.IsAtLeast(V2_4_0_0))
    {
        request->Version = 6;
    }
    else if (version.IsAtLeast(V2_1_0_0))
    {
        request->Version = 5;
    }
    else if (version.IsAtLeast(V2_0_0_0))
    {
        request->Version = 4;
    }
    else if (version.IsAtLeast(V0_11_0_0))
    {
        request->Version = 3;
    }
    else if (version.IsAtLeast(V0_10_2_0))
    {
        request->Version = 2;
    }
    else if (version.IsAtLeast(V0_8_2_0))
    {
        request->Version = 1;
    }

    return request;
}

void OffsetFetchRequest::setVersion(int16_t v)
{
    Version = v;
}

int OffsetFetchRequest::encode(PEncoder &pe)
{
    if (Version < 0 || Version > 7)
    {
        throw std::runtime_error("invalid or unsupported OffsetFetchRequest version field");
    }

    pe.putString(ConsumerGroup);

    if (partitions.empty() && Version >= 2)
    {
        pe.putArrayLength(-1);
    }
    else
    {
        pe.putArrayLength(static_cast<int32_t>(partitions.size()));
    }

    for (auto &entry : partitions)
    {
        pe.putString(entry.first);
        pe.putInt32Array(entry.second);
        pe.putEmptyTaggedFieldArray();
    }

    if (RequireStable && Version < 7)
    {
        throw std::runtime_error("requireStable is not supported. use version 7 or later");
    }

    if (Version >= 7)
    {
        pe.putBool(RequireStable);
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int OffsetFetchRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;
    int err = pd.getString(ConsumerGroup);
    if (err != 0)
    {
        return err;
    }

    int32_t partitionCount = 0;
    err = pd.getArrayLength(partitionCount);
    if (err != 0)
    {
        return err;
    }
    if ((partitionCount == 0 && version < 2) || partitionCount < 0)
    {
        return ErrInvalidPartitions;
    }

    partitions.clear();
    // partitions.reserve(partitionCount);
    for (int i = 0; i < partitionCount; ++i)
    {
        std::string topic;
        err = pd.getString(topic);
        if (err != 0)
        {
            return err;
        }
        std::vector<int32_t> parts;
        err = pd.getInt32Array(parts);
        if (err != 0)
        {
            return err;
        }

        int32_t _;
        err = pd.getEmptyTaggedFieldArray(_);
        if (err != 0)
        {
            return err;
        }
        partitions[topic] = std::move(parts);
    }

    if (Version >= 7)
    {
        pd.getBool(RequireStable);
    }

    int32_t _;
    err = pd.getEmptyTaggedFieldArray(_);
    if (err != 0)
    {
        return err;
    }
    return 0;
}

int16_t OffsetFetchRequest::key() const
{
    return apiKeyOffsetFetch;
}

int16_t OffsetFetchRequest::version() const
{
    return Version;
}

int16_t OffsetFetchRequest::headerVersion() const
{
    return Version >= 6 ? 2 : 1;
}

bool OffsetFetchRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 7;
}

bool OffsetFetchRequest::isFlexible() const
{
    return isFlexibleVersion(Version);
}

bool OffsetFetchRequest::isFlexibleVersion(int16_t version)
{
    return version >= 6;
}

KafkaVersion OffsetFetchRequest::requiredVersion() const
{
    switch (Version)
    {
    case 7:
        return V2_5_0_0;
    case 6:
        return V2_4_0_0;
    case 5:
        return V2_1_0_0;
    case 4:
        return V2_0_0_0;
    case 3:
        return V0_11_0_0;
    case 2:
        return V0_10_2_0;
    case 1:
    case 0:
        return V0_8_2_0;
    default:
        return V2_5_0_0;
    }
}

void OffsetFetchRequest::ZeroPartitions()
{
    if (partitions.empty() && Version >= 2)
    {
        partitions.clear();
    }
}

void OffsetFetchRequest::AddPartition(const std::string &topic, int32_t partitionID)
{
    partitions[topic].push_back(partitionID);
}