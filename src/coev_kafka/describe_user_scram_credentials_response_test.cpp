#include <gtest/gtest.h>
#include "describe_user_scram_credentials_response.h"

TEST(DescribeUserScramCredentialsResponseTest, VersionCompatibility) {
    DescribeUserScramCredentialsResponse response;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(DescribeUserScramCredentialsResponseTest, BasicFunctionality) {
    DescribeUserScramCredentialsResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}