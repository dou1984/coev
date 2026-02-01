#include "describe_configs_request.h"
#include <gtest/gtest.h>
#include "real_encoder.h"
#include "packet_decoder.h"

TEST(DescribeConfigsRequestTest, BasicFunctionality)
{
    // Test with version 0
    DescribeConfigsRequest request(0);
    EXPECT_TRUE(request.is_valid_version());
    EXPECT_EQ(request.version(), 0);
    EXPECT_EQ(request.key(), apiKeyDescribeConfigs);
    EXPECT_EQ(request.header_version(), 1);
}

TEST(DescribeConfigsRequestTest, VersionCompatibility)
{
    DescribeConfigsRequest request;

    // Test all valid versions (0-4)
    for (int16_t version = 0; version <= 4; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }

    // Test invalid versions
    request.set_version(-1);
    EXPECT_FALSE(request.is_valid_version());

    request.set_version(5);
    EXPECT_FALSE(request.is_valid_version());
}

TEST(DescribeConfigsRequestTest, EncodeEmptyRequest)
{
    DescribeConfigsRequest request(0);

    real_encoder encoder(1024);
    EXPECT_EQ(request.encode(encoder), ErrNoError);
}

TEST(DescribeConfigsRequestTest, EncodeWithResources)
{
    DescribeConfigsRequest request(0);

    // Add a resource
    auto resource = std::make_shared<ConfigResource>();
    resource->m_type = TopicResource;
    resource->m_name = "test-topic";
    resource->m_config_names = {"retention.ms", "cleanup.policy"};
    request.m_resources.push_back(resource);

    real_encoder encoder(1024);
    EXPECT_EQ(request.encode(encoder), ErrNoError);
}

TEST(DescribeConfigsRequestTest, EncodeWithIncludeSynonyms)
{
    // Test with version 1 which includes include_synonyms flag
    DescribeConfigsRequest request(1);
    request.m_include_synonyms = true;

    // Add a resource
    auto resource = std::make_shared<ConfigResource>();
    resource->m_type = TopicResource;
    resource->m_name = "test-topic";
    request.m_resources.push_back(resource);

    real_encoder encoder(1024);
    EXPECT_EQ(request.encode(encoder), ErrNoError);
}