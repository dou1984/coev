#include <gtest/gtest.h>
#include "metadata_request.h"
#include "metadata_response.h"

TEST(MetadataTest, RequestVersionCompatibility) {
    MetadataRequest request;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 10; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(MetadataTest, ResponseVersionCompatibility) {
    MetadataResponse response;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 10; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(MetadataTest, BasicFunctionality) {
    MetadataRequest request;
    request.set_version(7);
    EXPECT_EQ(7, request.version());
    
    MetadataResponse response;
    response.set_version(7);
    EXPECT_EQ(7, response.version());
}