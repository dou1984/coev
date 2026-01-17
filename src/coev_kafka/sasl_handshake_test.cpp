#include <gtest/gtest.h>
#include "sasl_handshake_request.h"
#include "sasl_handshake_response.h"

TEST(SaslHandshakeTest, RequestVersionCompatibility) {
    SaslHandshakeRequest request;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(SaslHandshakeTest, ResponseVersionCompatibility) {
    SaslHandshakeResponse response;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(SaslHandshakeTest, BasicFunctionality) {
    SaslHandshakeRequest request;
    request.set_version(1);
    EXPECT_EQ(1, request.version());
    
    SaslHandshakeResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}