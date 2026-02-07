#include "describe_groups_response.h"
#include <gtest/gtest.h>
#include "real_encoder.h"
#include "real_decoder.h"

TEST(DescribeGroupsResponseTest, BasicFunctionality)
{
    // Test with version 0
    DescribeGroupsResponse response;
    response.set_version(0);
    EXPECT_TRUE(response.is_valid_version());
    EXPECT_EQ(response.version(), 0);
    EXPECT_EQ(response.key(), apiKeyDescribeGroups);
}

TEST(DescribeGroupsResponseTest, VersionCompatibility)
{
    DescribeGroupsResponse response;

    // Test all valid versions (0-5)
    for (int16_t version = 0; version <= 5; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }

    // Test invalid versions
    response.set_version(-1);
    EXPECT_FALSE(response.is_valid_version());

    response.set_version(6);
    EXPECT_FALSE(response.is_valid_version());
}

TEST(DescribeGroupsResponseTest, EncodeEmptyResponse)
{
    DescribeGroupsResponse response;
    response.set_version(0);

    real_encoder encoder(1024);
    EXPECT_EQ(response.encode(encoder), ErrNoError);
}

TEST(DescribeGroupsResponseTest, EncodeWithGroups)
{
    DescribeGroupsResponse response;
    response.set_version(0);

    // Add a group
    GroupDescription group;
    group.m_error_code = ErrNoError;
    group.m_group_id = "test-group";
    group.m_state = "Stable";
    group.m_protocol_type = "consumer";
    group.m_protocol = "range";
    response.m_groups.push_back(group);

    real_encoder encoder(1024);
    EXPECT_EQ(response.encode(encoder), ErrNoError);
}

TEST(DescribeGroupsResponseTest, EncodeWithVersionSpecificFields)
{
    // Test with version 3 which includes authorized operations
    DescribeGroupsResponse response;
    response.set_version(3);

    // Add a group
    // auto group = std::make_shared<GroupDescription>();
    GroupDescription group;
    group.m_error_code = ErrNoError;
    group.m_group_id = "test-group";
    group.m_state = "Stable";
    group.m_protocol_type = "consumer";
    group.m_protocol = "range";
    group.m_authorized_operations = 0x0f; // All operations
    response.m_groups.push_back(group);

    real_encoder encoder(1024);
    EXPECT_EQ(response.encode(encoder), ErrNoError);
}