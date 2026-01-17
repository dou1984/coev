#include <gtest/gtest.h>
#include "end_txn_request.h"
#include "end_txn_response.h"

TEST(EndTxnTest, RequestVersionCompatibility) {
    EndTxnRequest request;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(EndTxnTest, ResponseVersionCompatibility) {
    EndTxnResponse response;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(EndTxnTest, BasicFunctionality) {
    EndTxnRequest request;
    request.set_version(1);
    EXPECT_EQ(1, request.version());
    
    EndTxnResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}