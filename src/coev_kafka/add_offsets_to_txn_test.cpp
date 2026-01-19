#include <gtest/gtest.h>
#include "add_offsets_to_txn_request.h"
#include "add_offsets_to_txn_response.h"

TEST(AddOffsetsToTxnTest, RequestVersionCompatibility)
{
    AddOffsetsToTxnRequest request;

    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(AddOffsetsToTxnTest, ResponseVersionCompatibility)
{
    AddOffsetsToTxnResponse response;

    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(AddOffsetsToTxnTest, BasicFunctionality)
{
    AddOffsetsToTxnRequest request;
    request.set_version(1);
    EXPECT_EQ(1, request.version());

    AddOffsetsToTxnResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}