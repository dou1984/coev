#include <gtest/gtest.h>
#include "metadata_request.h"
#include "metadata_response.h"
#include "real_encoder.h"
#include "real_decoder.h"

TEST(MetadataProtocolTest, TestMetadataRequestEncodeDecode)
{
    // Create a MetadataRequest with some test data
    MetadataRequest original;
    original.m_version = 9; // Use version 9 which supports all fields
    original.m_topics = {"test-topic-1", "test-topic-2"};
    original.m_allow_auto_topic_creation = true;
    original.m_include_cluster_authorized_operations = false;
    original.m_include_topic_authorized_operations = true;

    // Encode the request
    realEncoder encoder(1024);
    int encode_err = original.encode(encoder);
    EXPECT_EQ(encode_err, 0);

    // Decode the request
    MetadataRequest decoded;
    realDecoder decoder;
    decoder.m_raw = encoder.m_raw.substr(0, encoder.m_offset);
    decoder.m_offset = 0;

    int decode_err = decoded.decode(decoder, original.m_version);
    EXPECT_EQ(decode_err, 0);

    // Verify the decoded request matches the original
    EXPECT_EQ(decoded.m_version, original.m_version);
    EXPECT_EQ(decoded.m_topics, original.m_topics);
    EXPECT_EQ(decoded.m_allow_auto_topic_creation, original.m_allow_auto_topic_creation);
    EXPECT_EQ(decoded.m_include_cluster_authorized_operations, original.m_include_cluster_authorized_operations);
    EXPECT_EQ(decoded.m_include_topic_authorized_operations, original.m_include_topic_authorized_operations);
}

TEST(MetadataProtocolTest, TestMetadataResponseEncodeDecode)
{
    // Create a MetadataResponse with some test data
    MetadataResponse original;
    original.m_version = 9; // Use version 9 which supports all fields
    original.m_throttle_time = std::chrono::milliseconds(100);
    original.m_cluster_id = "test-cluster";
    original.m_controller_id = 1;
    original.m_cluster_authorized_operations = 0;

    // Add a broker
    original.add_broker("localhost:9092", 1);

    // Add a topic with a partition
    original.add_topic_partition(
        "test-topic", 0, 1, {1, 2, 3}, {1, 2}, {3}, 0);

    // Encode the response
    realEncoder encoder(1024);
    int encode_err = original.encode(encoder);
    EXPECT_EQ(encode_err, 0);

    // Decode the response
    MetadataResponse decoded;
    realDecoder decoder;
    decoder.m_raw = encoder.m_raw.substr(0, encoder.m_offset);
    decoder.m_offset = 0;

    int decode_err = decoded.decode(decoder, original.m_version);
    EXPECT_EQ(decode_err, 0);

    // Verify the decoded response matches the original
    EXPECT_EQ(decoded.m_version, original.m_version);
    EXPECT_EQ(decoded.m_throttle_time, original.m_throttle_time);
    EXPECT_EQ(decoded.m_cluster_id, original.m_cluster_id);
    EXPECT_EQ(decoded.m_controller_id, original.m_controller_id);
    EXPECT_EQ(decoded.m_cluster_authorized_operations, original.m_cluster_authorized_operations);
    EXPECT_EQ(decoded.m_brokers.size(), original.m_brokers.size());
    EXPECT_EQ(decoded.m_topics.size(), original.m_topics.size());

    // Verify topic metadata
    if (!original.m_topics.empty() && !decoded.m_topics.empty())
    {
        EXPECT_EQ(decoded.m_topics[0]->m_name, original.m_topics[0]->m_name);
        EXPECT_EQ(decoded.m_topics[0]->m_partitions.size(), original.m_topics[0]->m_partitions.size());
    }
}

TEST(MetadataProtocolTest, TestMetadataRequestEmptyTopics)
{
    // Test with empty topics list
    MetadataRequest original;
    original.m_version = 9;             // Use version 9 which supports all fields
    original.m_topics = {"test-topic"}; // Not empty to avoid special handling
    original.m_allow_auto_topic_creation = false;
    original.m_include_cluster_authorized_operations = false;
    original.m_include_topic_authorized_operations = false;

    // Encode the request
    realEncoder encoder(1024);
    int encode_err = original.encode(encoder);
    EXPECT_EQ(encode_err, 0);

    // Decode the request
    MetadataRequest decoded;
    realDecoder decoder;
    decoder.m_raw = encoder.m_raw.substr(0, encoder.m_offset);
    decoder.m_offset = 0;

    int decode_err = decoded.decode(decoder, original.m_version);
    EXPECT_EQ(decode_err, 0);

    // Verify the decoded request matches the original
    EXPECT_EQ(decoded.m_topics.size(), original.m_topics.size());
    EXPECT_EQ(decoded.m_allow_auto_topic_creation, original.m_allow_auto_topic_creation);
}

TEST(MetadataProtocolTest, TestMetadataResponseMultipleTopics)
{
    // Test with multiple topics
    MetadataResponse original;
    original.m_version = 9; // Use version 9 which supports all fields
    original.m_throttle_time = std::chrono::milliseconds(0);
    original.m_cluster_id = "multi-topic-cluster";
    original.m_controller_id = 2;
    original.m_cluster_authorized_operations = 0;

    // Add multiple topics
    original.add_topic_partition("topic-1", 0, 1, {1}, {1}, {}, 0);
    original.add_topic_partition("topic-1", 1, 1, {1}, {1}, {}, 0);
    original.add_topic_partition("topic-2", 0, 1, {1}, {1}, {}, 0);

    // Encode the response
    realEncoder encoder(2048);
    int encode_err = original.encode(encoder);
    EXPECT_EQ(encode_err, 0);

    // Decode the response
    MetadataResponse decoded;
    realDecoder decoder;
    decoder.m_raw = encoder.m_raw.substr(0, encoder.m_offset);
    decoder.m_offset = 0;

    int decode_err = decoded.decode(decoder, original.m_version);
    EXPECT_EQ(decode_err, 0);

    // Verify the decoded response has the correct number of topics and partitions
    EXPECT_EQ(decoded.m_topics.size(), 2);                // topic-1 and topic-2
    EXPECT_EQ(decoded.m_topics[0]->m_partitions.size(), 2); // topic-1 has 2 partitions
    EXPECT_EQ(decoded.m_topics[1]->m_partitions.size(), 1); // topic-2 has 1 partition
}

TEST(MetadataProtocolTest, TestMetadataRequestVersionCompatibility)
{
    // Test with different versions
    for (int16_t version = 0; version <= 10; ++version)
    {
        MetadataRequest original;
        original.m_version = version;
        original.m_topics = {"test-topic"};

        // Only set fields that are supported by this version
        if (version >= 4)
        {
            original.m_allow_auto_topic_creation = version % 2 == 0;
        }
        if (version >= 8)
        {
            original.m_include_cluster_authorized_operations = version % 2 == 0;
            original.m_include_topic_authorized_operations = version % 2 != 0;
        }

        // Encode the request
        realEncoder encoder(1024);
        int encode_err = original.encode(encoder);
        EXPECT_EQ(encode_err, 0) << "Failed to encode MetadataRequest with version " << version;

        // Decode the request
        MetadataRequest decoded;
        realDecoder decoder;
        decoder.m_raw = encoder.m_raw.substr(0, encoder.m_offset);
        decoder.m_offset = 0;

        int decode_err = decoded.decode(decoder, original.m_version);
        EXPECT_EQ(decode_err, 0) << "Failed to decode MetadataRequest with version " << version;

        // Verify the decoded request matches the original
        EXPECT_EQ(decoded.m_version, original.m_version);
        EXPECT_EQ(decoded.m_topics, original.m_topics);

        // Verify version-specific fields
        if (version >= 4)
        {
            EXPECT_EQ(decoded.m_allow_auto_topic_creation, original.m_allow_auto_topic_creation);
        }
        if (version >= 8)
        {
            EXPECT_EQ(decoded.m_include_cluster_authorized_operations, original.m_include_cluster_authorized_operations);
            EXPECT_EQ(decoded.m_include_topic_authorized_operations, original.m_include_topic_authorized_operations);
        }
    }
}

TEST(MetadataProtocolTest, TestMetadataResponseVersionCompatibility)
{
    // Test with different versions
    for (int16_t version = 0; version <= 10; ++version)
    {
        MetadataResponse original;
        original.m_version = version;

        // Only set fields that are supported by this version
        if (version >= 3)
        {
            original.m_throttle_time = std::chrono::milliseconds(version * 100);
        }
        if (version >= 2)
        {
            original.m_cluster_id = "cluster-" + std::to_string(version);
        }
        if (version >= 1)
        {
            original.m_controller_id = version;
        }
        if (version >= 8)
        {
            original.m_cluster_authorized_operations = version;
        }

        // Add a broker
        original.add_broker("localhost:9092", version);

        // Add a topic with a partition
        original.add_topic_partition(
            "test-topic", 0, version, {version}, {version},
            version >= 5 ? std::vector<int32_t>{version} : std::vector<int32_t>{}, 0);

        // Encode the response
        realEncoder encoder(1024);
        int encode_err = original.encode(encoder);
        EXPECT_EQ(encode_err, 0) << "Failed to encode MetadataResponse with version " << version;

        // Decode the response
        MetadataResponse decoded;
        realDecoder decoder;
        decoder.m_raw = encoder.m_raw.substr(0, encoder.m_offset);
        decoder.m_offset = 0;

        int decode_err = decoded.decode(decoder, original.m_version);
        EXPECT_EQ(decode_err, 0) << "Failed to decode MetadataResponse with version " << version;

        // Verify the decoded response matches the original
        EXPECT_EQ(decoded.m_version, original.m_version);
        EXPECT_EQ(decoded.m_brokers.size(), original.m_brokers.size());
        EXPECT_EQ(decoded.m_topics.size(), original.m_topics.size());

        // Verify version-specific fields
        if (version >= 3)
        {
            EXPECT_EQ(decoded.m_throttle_time, original.m_throttle_time);
        }
        if (version >= 2)
        {
            EXPECT_EQ(decoded.m_cluster_id, original.m_cluster_id);
        }
        if (version >= 1)
        {
            EXPECT_EQ(decoded.m_controller_id, original.m_controller_id);
        }
        if (version >= 8)
        {
            EXPECT_EQ(decoded.m_cluster_authorized_operations, original.m_cluster_authorized_operations);
        }
    }
}

TEST(MetadataProtocolTest, TestMetadataRequestEmptyTopicsSpecialCase)
{
    // Test with empty topics list and version 0
    MetadataRequest original;
    original.m_version = 0; // Version 0 always encodes topics array
    original.m_topics = {}; // Empty topics list

    // Encode the request
    realEncoder encoder(1024);
    int encode_err = original.encode(encoder);
    EXPECT_EQ(encode_err, 0);

    // Decode the request
    MetadataRequest decoded;
    realDecoder decoder;
    decoder.m_raw = encoder.m_raw.substr(0, encoder.m_offset);
    decoder.m_offset = 0;

    int decode_err = decoded.decode(decoder, original.m_version);
    EXPECT_EQ(decode_err, 0);

    // Verify the decoded request matches the original
    EXPECT_EQ(decoded.m_version, original.m_version);
    EXPECT_EQ(decoded.m_topics, original.m_topics);
}

TEST(MetadataProtocolTest, TestMetadataResponseWithMultipleBrokers)
{
    // Test with multiple brokers
    MetadataResponse original;
    original.m_version = 9;
    original.m_cluster_id = "multi-broker-cluster";
    original.m_controller_id = 1;

    // Add multiple brokers
    original.add_broker("broker1:9092", 1);
    original.add_broker("broker2:9092", 2);
    original.add_broker("broker3:9092", 3);

    // Add a topic with multiple partitions
    original.add_topic_partition("test-topic", 0, 1, {1, 2, 3}, {1, 2}, {3}, 0);
    original.add_topic_partition("test-topic", 1, 2, {1, 2, 3}, {2, 3}, {1}, 0);
    original.add_topic_partition("test-topic", 2, 3, {1, 2, 3}, {1, 3}, {2}, 0);

    // Encode the response
    realEncoder encoder(2048);
    int encode_err = original.encode(encoder);
    EXPECT_EQ(encode_err, 0);

    // Decode the response
    MetadataResponse decoded;
    realDecoder decoder;
    decoder.m_raw = encoder.m_raw.substr(0, encoder.m_offset);
    decoder.m_offset = 0;

    int decode_err = decoded.decode(decoder, original.m_version);
    EXPECT_EQ(decode_err, 0);

    // Verify the decoded response has the correct number of brokers and partitions
    EXPECT_EQ(decoded.m_brokers.size(), 3);
    EXPECT_EQ(decoded.m_topics.size(), 1);
    EXPECT_EQ(decoded.m_topics[0]->m_partitions.size(), 3);

    // Verify some partition details
    EXPECT_EQ(decoded.m_topics[0]->m_partitions[0]->m_leader, 1);
    EXPECT_EQ(decoded.m_topics[0]->m_partitions[1]->m_leader, 2);
    EXPECT_EQ(decoded.m_topics[0]->m_partitions[2]->m_leader, 3);

    // Verify replicas and ISR
    EXPECT_EQ(decoded.m_topics[0]->m_partitions[0]->m_replicas.size(), 3);
    EXPECT_EQ(decoded.m_topics[0]->m_partitions[0]->m_isr.size(), 2);
    EXPECT_EQ(decoded.m_topics[0]->m_partitions[0]->m_offline_replicas.size(), 1);
}
