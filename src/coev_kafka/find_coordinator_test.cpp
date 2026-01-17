#include <gtest/gtest.h>
#include "find_coordinator_request.h"
#include "find_coordinator_response.h"

TEST(FindCoordinatorTest, RequestVersionCompatibility) {
    FindCoordinatorRequest request;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 3; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(FindCoordinatorTest, ResponseVersionCompatibility) {
    FindCoordinatorResponse response;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 3; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(FindCoordinatorTest, BasicFunctionality) {
    FindCoordinatorRequest request;
    request.set_version(1);
    EXPECT_EQ(1, request.version());
    
    FindCoordinatorResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}