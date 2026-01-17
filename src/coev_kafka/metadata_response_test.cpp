#include <gtest/gtest.h>
#include "metadata_response.h"

TEST(MetadataResponseTest, VersionCompatibility) {
    MetadataResponse response;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 10; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(MetadataResponseTest, BasicFunctionality) {
    MetadataResponse response;
    response.set_version(7);
    EXPECT_EQ(7, response.version());
}