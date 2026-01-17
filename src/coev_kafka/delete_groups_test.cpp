#include <gtest/gtest.h>
#include "delete_groups_request.h"
#include "delete_groups_response.h"

TEST(DeleteGroupsTest, RequestVersionCompatibility) {
    DeleteGroupsRequest request;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(DeleteGroupsTest, ResponseVersionCompatibility) {
    DeleteGroupsResponse response;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(DeleteGroupsTest, BasicFunctionality) {
    DeleteGroupsRequest request;
    request.set_version(1);
    EXPECT_EQ(1, request.version());
    
    DeleteGroupsResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}