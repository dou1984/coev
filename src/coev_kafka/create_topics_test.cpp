#include <gtest/gtest.h>
#include "create_topics_request.h"
#include "create_topics_response.h"

TEST(CreateTopicsTest, RequestVersionCompatibility)
{
    CreateTopicsRequest request;

    // Test all supported versions
    for (int16_t version = 0; version <= 6; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(CreateTopicsTest, ResponseVersionCompatibility)
{
    CreateTopicsResponse response;

    // Test all supported versions
    for (int16_t version = 0; version <= 6; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(CreateTopicsTest, BasicFunctionality)
{
    CreateTopicsRequest request;
    request.set_version(3);
    EXPECT_EQ(3, request.version());

    CreateTopicsResponse response;
    response.set_version(3);
    EXPECT_EQ(3, response.version());
}