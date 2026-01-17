#include <gtest/gtest.h>
#include "create_topics_response.h"

TEST(CreateTopicsResponseTest, VersionCompatibility) {
    CreateTopicsResponse response;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 6; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(CreateTopicsResponseTest, BasicFunctionality) {
    CreateTopicsResponse response;
    response.set_version(3);
    EXPECT_EQ(3, response.version());
}