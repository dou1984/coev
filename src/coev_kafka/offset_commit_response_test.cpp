#include <gtest/gtest.h>
#include "offset_commit_response.h"

TEST(OffsetCommitResponseTest, VersionCompatibility)
{
    OffsetCommitResponse response;

    // Test all supported versions
    for (int16_t version = 0; version <= 7; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(OffsetCommitResponseTest, BasicFunctionality)
{
    OffsetCommitResponse response;
    response.set_version(2);
    EXPECT_EQ(2, response.version());
}