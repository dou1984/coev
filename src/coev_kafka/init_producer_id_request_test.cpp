#include <gtest/gtest.h>
#include "init_producer_id_request.h"

TEST(InitProducerIdRequestTest, VersionCompatibility) {
    InitProducerIDRequest request;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 2; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(InitProducerIdRequestTest, BasicFunctionality) {
    InitProducerIDRequest request;
    request.set_version(1);
    EXPECT_EQ(1, request.version());
}