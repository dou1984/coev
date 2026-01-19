#include <gtest/gtest.h>
#include "delete_topics_request.h"
#include "delete_topics_response.h"

TEST(DeleteTopicsTest, RequestVersionCompatibility)
{
    DeleteTopicsRequest request;

    // Test all supported versions
    for (int16_t version = 0; version <= 3; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(DeleteTopicsTest, ResponseVersionCompatibility)
{
    DeleteTopicsResponse response;

    // Test all supported versions
    for (int16_t version = 0; version <= 3; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(DeleteTopicsTest, BasicFunctionality)
{
    DeleteTopicsRequest request;
    request.set_version(1);
    EXPECT_EQ(1, request.version());

    DeleteTopicsResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}