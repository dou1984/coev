#include <gtest/gtest.h>
#include "sync_group_request.h"
#include "sync_group_response.h"

TEST(SyncGroupTest, RequestVersionCompatibility)
{
    SyncGroupRequest request;

    // Test all supported versions
    for (int16_t version = 0; version <= 4; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(SyncGroupTest, ResponseVersionCompatibility)
{
    SyncGroupResponse response;

    // Test all supported versions
    for (int16_t version = 0; version <= 4; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(SyncGroupTest, BasicFunctionality)
{
    SyncGroupRequest request;
    request.set_version(2);
    EXPECT_EQ(2, request.version());

    SyncGroupResponse response;
    response.set_version(2);
    EXPECT_EQ(2, response.version());
}