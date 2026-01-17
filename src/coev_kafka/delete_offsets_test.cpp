#include <gtest/gtest.h>
#include "delete_offsets_request.h"
#include "delete_offsets_response.h"

TEST(DeleteOffsetsTest, RequestVersionCompatibility) {
    DeleteOffsetsRequest request;
    for (int16_t version = 0; version <= 2; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(DeleteOffsetsTest, ResponseVersionCompatibility) {
    DeleteOffsetsResponse response;
    for (int16_t version = 0; version <= 2; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(DeleteOffsetsTest, BasicFunctionality) {
    DeleteOffsetsRequest request;
    request.set_version(1);
    EXPECT_EQ(request.version(), 1);
    EXPECT_EQ(request.key(), 47);
    
    DeleteOffsetsResponse response;
    response.set_version(1);
    EXPECT_EQ(response.version(), 1);
    EXPECT_EQ(response.key(), 47);
}