#include <gtest/gtest.h>
#include "offset_fetch_request.h"
#include "offset_fetch_response.h"

TEST(OffsetFetchTest, RequestVersionCompatibility) {
    OffsetFetchRequest request;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 7; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(OffsetFetchTest, ResponseVersionCompatibility) {
    OffsetFetchResponse response;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 7; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(OffsetFetchTest, BasicFunctionality) {
    OffsetFetchRequest request;
    request.set_version(4);
    EXPECT_EQ(4, request.version());
    
    OffsetFetchResponse response;
    response.set_version(4);
    EXPECT_EQ(4, response.version());
}