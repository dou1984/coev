/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <gtest/gtest.h>
#include "list_partition_reassignments_request.h"
using namespace coev::kafka;
TEST(ListPartitionReassignmentsRequestTest, VersionCompatibility)
{
    ListPartitionReassignmentsRequest request;

    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(ListPartitionReassignmentsRequestTest, BasicFunctionality)
{
    ListPartitionReassignmentsRequest request;
    request.set_version(1);
    EXPECT_EQ(1, request.version());
}