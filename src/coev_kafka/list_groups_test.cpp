#include <gtest/gtest.h>
#include "list_groups_request.h"
#include "list_groups_response.h"

TEST(ListGroupsTest, RequestVersionCompatibility) {
    ListGroupsRequest request;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 2; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(ListGroupsTest, ResponseVersionCompatibility) {
    ListGroupsResponse response;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 2; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(ListGroupsTest, BasicFunctionality) {
    ListGroupsRequest request;
    request.set_version(1);
    EXPECT_EQ(1, request.version());
    
    ListGroupsResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}