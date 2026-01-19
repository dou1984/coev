#include <gtest/gtest.h>
#include "add_partitions_to_txn_response.h"

TEST(AddPartitionsToTxnResponseTest, VersionCompatibility)
{
    AddPartitionsToTxnResponse response;

    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(AddPartitionsToTxnResponseTest, BasicFunctionality)
{
    AddPartitionsToTxnResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}