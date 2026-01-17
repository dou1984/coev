#include <gtest/gtest.h>
#include "offset_commit_request.h"
#include "offset_commit_response.h"

TEST(OffsetCommitTest, RequestVersionCompatibility) {
    OffsetCommitRequest request;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 7; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(OffsetCommitTest, ResponseVersionCompatibility) {
    OffsetCommitResponse response;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 7; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(OffsetCommitTest, BasicFunctionality) {
    OffsetCommitRequest request;
    request.set_version(2);
    EXPECT_EQ(2, request.version());
    
    OffsetCommitResponse response;
    response.set_version(2);
    EXPECT_EQ(2, response.version());
}