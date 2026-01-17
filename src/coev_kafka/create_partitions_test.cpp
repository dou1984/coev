#include <gtest/gtest.h>
#include "create_partitions_request.h"
#include "create_partitions_response.h"

TEST(CreatePartitionsTest, RequestVersionCompatibility) {
    CreatePartitionsRequest request;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 3; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(CreatePartitionsTest, ResponseVersionCompatibility) {
    CreatePartitionsResponse response;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 3; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(CreatePartitionsTest, BasicFunctionality) {
    CreatePartitionsRequest request;
    request.set_version(1);
    EXPECT_EQ(1, request.version());
    
    CreatePartitionsResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}