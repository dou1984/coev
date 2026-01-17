#include <gtest/gtest.h>
#include "acl_create_request.h"
#include "acl_create_response.h"

TEST(AclCreateTest, RequestVersionCompatibility) {
    CreateAclsRequest request;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(AclCreateTest, ResponseVersionCompatibility) {
    CreateAclsResponse response;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(AclCreateTest, BasicFunctionality) {
    CreateAclsRequest request;
    request.set_version(1);
    EXPECT_EQ(1, request.version());
    
    CreateAclsResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}