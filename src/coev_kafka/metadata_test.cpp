#include <gtest/gtest.h>
#include "metadata_request.h"
#include "metadata_response.h"

TEST(MetadataTest, RequestVersionCompatibility)
{
    MetadataRequest request;

    // Test different versions
    for (int16_t version = 0; version <= 4; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(MetadataTest, ResponseVersionCompatibility)
{
    MetadataResponse response;

    // Test different versions
    for (int16_t version = 0; version <= 4; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(MetadataTest, BasicFunctionality)
{
    MetadataRequest request;
    request.set_version(4);
    EXPECT_EQ(4, request.version());

    MetadataResponse response;
    response.set_version(4);
    EXPECT_EQ(4, response.version());
}