/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <gtest/gtest.h>
#include "add_partitions_to_txn_request.h"
#include "add_partitions_to_txn_response.h"

using namespace coev::kafka;

TEST(AddPartitionsToTxnTest, RequestVersionCompatibility)
{
    AddPartitionsToTxnRequest request;

    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(AddPartitionsToTxnTest, ResponseVersionCompatibility)
{
    AddPartitionsToTxnResponse response;

    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(AddPartitionsToTxnTest, BasicFunctionality)
{
    AddPartitionsToTxnRequest request;
    request.set_version(1);
    EXPECT_EQ(1, request.version());

    AddPartitionsToTxnResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}