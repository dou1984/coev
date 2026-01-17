#include <gtest/gtest.h>
#include "delete_topics_response.h"

TEST(DeleteTopicsResponseTest, VersionCompatibility) {
    DeleteTopicsResponse response;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 3; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(DeleteTopicsResponseTest, BasicFunctionality) {
    DeleteTopicsResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}