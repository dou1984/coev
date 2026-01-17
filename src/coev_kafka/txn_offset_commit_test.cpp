#include <gtest/gtest.h>
#include "txn_offset_commit_request.h"
#include "txn_offset_commit_response.h"

TEST(TxnOffsetCommitTest, RequestVersionCompatibility) {
    TxnOffsetCommitRequest request;
    for (int16_t version = 0; version <= 2; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(TxnOffsetCommitTest, ResponseVersionCompatibility) {
    TxnOffsetCommitResponse response;
    for (int16_t version = 0; version <= 2; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(TxnOffsetCommitTest, BasicFunctionality) {
    TxnOffsetCommitRequest request;
    request.set_version(1);
    EXPECT_EQ(request.version(), 1);
    EXPECT_EQ(request.key(), 28);
    
    TxnOffsetCommitResponse response;
    response.set_version(1);
    EXPECT_EQ(response.version(), 1);
    EXPECT_EQ(response.key(), 28);
}