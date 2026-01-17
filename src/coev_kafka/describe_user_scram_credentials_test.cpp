#include <gtest/gtest.h>
#include "describe_user_scram_credentials_request.h"
#include "describe_user_scram_credentials_response.h"

TEST(DescribeUserScramCredentialsTest, RequestVersionCompatibility) {
    DescribeUserScramCredentialsRequest request;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(DescribeUserScramCredentialsTest, ResponseVersionCompatibility) {
    DescribeUserScramCredentialsResponse response;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(DescribeUserScramCredentialsTest, BasicFunctionality) {
    DescribeUserScramCredentialsRequest request;
    request.set_version(1);
    EXPECT_EQ(1, request.version());
    
    DescribeUserScramCredentialsResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}