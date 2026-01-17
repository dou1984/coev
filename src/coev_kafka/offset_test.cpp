#include <gtest/gtest.h>
#include "offset_request.h"
#include "offset_response.h"

TEST(OffsetTest, RequestVersionCompatibility) {
    OffsetRequest request;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 5; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(OffsetTest, ResponseVersionCompatibility) {
    OffsetResponse response;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 5; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(OffsetTest, BasicFunctionality) {
    OffsetRequest request;
    request.set_version(2);
    EXPECT_EQ(2, request.version());
    
    OffsetResponse response;
    response.set_version(2);
    EXPECT_EQ(2, response.version());
}