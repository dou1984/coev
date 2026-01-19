#include <gtest/gtest.h>
#include "sync_group_request.h"

TEST(SyncGroupRequestTest, VersionCompatibility)
{
    SyncGroupRequest request;

    // Test all supported versions
    for (int16_t version = 0; version <= 5; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(SyncGroupRequestTest, BasicFunctionality)
{
    SyncGroupRequest request;
    request.set_version(1);
    EXPECT_EQ(1, request.version());
}