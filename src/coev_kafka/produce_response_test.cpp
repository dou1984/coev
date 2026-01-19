#include <gtest/gtest.h>
#include "produce_response.h"

TEST(ProduceResponseTest, VersionCompatibility)
{
    ProduceResponse response;

    // Test all supported versions
    for (int16_t version = 0; version <= 7; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(ProduceResponseTest, BasicFunctionality)
{
    ProduceResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}