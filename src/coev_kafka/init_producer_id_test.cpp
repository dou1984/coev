#include <gtest/gtest.h>
#include "init_producer_id_request.h"
#include "init_producer_id_response.h"

TEST(InitProducerIdTest, RequestVersionCompatibility)
{
    InitProducerIDRequest request;

    // Test all supported versions
    for (int16_t version = 0; version <= 2; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(InitProducerIdTest, ResponseVersionCompatibility)
{
    InitProducerIDResponse response;

    // Test all supported versions
    for (int16_t version = 0; version <= 2; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(InitProducerIdTest, BasicFunctionality)
{
    InitProducerIDRequest request;
    request.set_version(1);
    EXPECT_EQ(1, request.version());

    InitProducerIDResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}