#include <gtest/gtest.h>
#include "sasl_authenticate_request.h"
#include "sasl_authenticate_response.h"

TEST(SaslAuthenticateTest, RequestVersionCompatibility)
{
    SaslAuthenticateRequest request;

    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(SaslAuthenticateTest, ResponseVersionCompatibility)
{
    SaslAuthenticateResponse response;

    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(SaslAuthenticateTest, BasicFunctionality)
{
    SaslAuthenticateRequest request;
    request.set_version(1);
    EXPECT_EQ(1, request.version());

    SaslAuthenticateResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}