#include <gtest/gtest.h>
#include "fetch_response.h"

TEST(FetchResponseTest, VersionCompatibility) {
    FetchResponse response;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 15; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(FetchResponseTest, BasicFunctionality) {
    FetchResponse response;
    response.set_version(10);
    EXPECT_EQ(10, response.version());
}