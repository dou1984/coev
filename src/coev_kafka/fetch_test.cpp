#include <gtest/gtest.h>
#include "fetch_request.h"
#include "fetch_response.h"

TEST(FetchTest, RequestVersionCompatibility) {
    FetchRequest request;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 15; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(FetchTest, ResponseVersionCompatibility) {
    FetchResponse response;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 15; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(FetchTest, BasicFunctionality) {
    FetchRequest request;
    request.set_version(10);
    EXPECT_EQ(10, request.version());
    
    FetchResponse response;
    response.set_version(10);
    EXPECT_EQ(10, response.version());
}