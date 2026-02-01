#include "sasl_authenticate_request.h"
#include <gtest/gtest.h>
#include "real_encoder.h"
#include "packet_decoder.h"

TEST(SaslAuthenticateRequestTest, BasicFunctionality)
{
    // Test with version 0
    SaslAuthenticateRequest request(0);
    EXPECT_TRUE(request.is_valid_version());
    EXPECT_EQ(request.version(), 0);
    EXPECT_EQ(request.key(), apiKeySASLAuth);
    EXPECT_EQ(request.header_version(), 1);
}

TEST(SaslAuthenticateRequestTest, VersionCompatibility)
{
    SaslAuthenticateRequest request;

    // Test all valid versions (0-1)
    for (int16_t version = 0; version <= 1; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }

    // Test invalid versions
    request.set_version(-1);
    EXPECT_FALSE(request.is_valid_version());

    request.set_version(2);
    EXPECT_FALSE(request.is_valid_version());
}

TEST(SaslAuthenticateRequestTest, EncodeWithAuthBytes)
{
    SaslAuthenticateRequest request(0);
    request.m_sasl_auth_bytes = "test-auth-bytes";

    real_encoder encoder(1024);
    EXPECT_EQ(request.encode(encoder), 0);
}

TEST(SaslAuthenticateRequestTest, EncodeWithVersion1)
{
    SaslAuthenticateRequest request(1);
    request.m_sasl_auth_bytes = "test-auth-bytes";

    real_encoder encoder(1024);
    EXPECT_EQ(request.encode(encoder), 0);
}