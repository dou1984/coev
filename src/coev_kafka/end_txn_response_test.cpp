#include <gtest/gtest.h>
#include "end_txn_response.h"

TEST(EndTxnResponseTest, VersionCompatibility) {
    EndTxnResponse response;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(EndTxnResponseTest, BasicFunctionality) {
    EndTxnResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}