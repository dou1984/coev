#include <gtest/gtest.h>
#include "alter_user_scram_credentials_request.h"
#include "alter_user_scram_credentials_response.h"

TEST(AlterUserScramCredentialsTest, RequestVersionCompatibility) {
    AlterUserScramCredentialsRequest request;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(AlterUserScramCredentialsTest, ResponseVersionCompatibility) {
    AlterUserScramCredentialsResponse response;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(AlterUserScramCredentialsTest, BasicFunctionality) {
    AlterUserScramCredentialsRequest request;
    request.set_version(1);
    EXPECT_EQ(1, request.version());
    
    AlterUserScramCredentialsResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}