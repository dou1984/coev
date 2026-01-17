#include "describe_client_quotas_response.h"
#include <gtest/gtest.h>
#include "real_encoder.h"
#include "packet_decoder.h"

TEST(DescribeClientQuotasResponseTest, BasicFunctionality) {
    // Test with version 0
    DescribeClientQuotasResponse response;
    response.set_version(0);
    EXPECT_TRUE(response.is_valid_version());
    EXPECT_EQ(response.version(), 0);
    EXPECT_EQ(response.key(), apiKeyDescribeClientQuotas);
}

TEST(DescribeClientQuotasResponseTest, VersionCompatibility) {
    DescribeClientQuotasResponse response;
    
    // Test all valid versions (0-1)
    for (int16_t version = 0; version <= 1; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
    
    // Test invalid versions
    response.set_version(-1);
    EXPECT_FALSE(response.is_valid_version());
    
    response.set_version(2);
    EXPECT_FALSE(response.is_valid_version());
}

TEST(DescribeClientQuotasResponseTest, EncodeEmptyResponse) {
    DescribeClientQuotasResponse response;
    response.set_version(0);
    
    realEncoder encoder(1024);
    EXPECT_EQ(response.encode(encoder), ErrNoError);
}

TEST(DescribeClientQuotasResponseTest, EncodeWithThrottleTime) {
    DescribeClientQuotasResponse response;
    response.set_version(0);
    response.m_throttle_time = std::chrono::milliseconds(100);
    response.m_error_code = ErrNoError;
    
    realEncoder encoder(1024);
    EXPECT_EQ(response.encode(encoder), ErrNoError);
}

TEST(DescribeClientQuotasResponseTest, EncodeWithError) {
    DescribeClientQuotasResponse response;
    response.set_version(0);
    response.m_throttle_time = std::chrono::milliseconds(100);
    response.m_error_code = ErrInvalidRequest;
    response.m_error_msg = "Invalid request";
    
    realEncoder encoder(1024);
    EXPECT_EQ(response.encode(encoder), ErrNoError);
}