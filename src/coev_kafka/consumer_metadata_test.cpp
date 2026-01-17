#include <gtest/gtest.h>
#include "consumer_metadata_request.h"
#include "consumer_metadata_response.h"

TEST(ConsumerMetadataTest, RequestVersionCompatibility) {
    ConsumerMetadataRequest request;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(ConsumerMetadataTest, ResponseVersionCompatibility) {
    ConsumerMetadataResponse response;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(ConsumerMetadataTest, BasicFunctionality) {
    ConsumerMetadataRequest request;
    request.set_version(1);
    EXPECT_EQ(1, request.version());
    
    ConsumerMetadataResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}