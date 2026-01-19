#include <gtest/gtest.h>
#include "produce_request.h"
#include "produce_response.h"

TEST(ProduceProtocolTest, RequestVersionCompatibility)
{
    ProduceRequest request;

    // Test all supported versions
    for (int16_t version = 0; version <= 9; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(ProduceProtocolTest, ResponseVersionCompatibility)
{
    ProduceResponse response;

    // Test all supported versions
    for (int16_t version = 0; version <= 9; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(ProduceProtocolTest, BasicFunctionality)
{
    ProduceRequest request;
    request.set_version(5);
    EXPECT_EQ(5, request.version());

    ProduceResponse response;
    response.set_version(5);
    EXPECT_EQ(5, response.version());
}