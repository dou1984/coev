#include "offset_fetch_response.h"
#include <gtest/gtest.h>
#include "real_encoder.h"
#include "real_decoder.h"
#include "api_versions.h"

TEST(OffsetFetchResponseTest, BasicFunctionality)
{
    // Test with version 0
    OffsetFetchResponse response;
    response.set_version(0);
    EXPECT_TRUE(response.is_valid_version());
    EXPECT_EQ(response.version(), 0);
    EXPECT_EQ(response.key(), apiKeyOffsetFetch);
}

TEST(OffsetFetchResponseTest, VersionCompatibility)
{
    OffsetFetchResponse response;

    // Test all valid versions (0-7)
    for (int16_t version = 0; version <= 7; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }

    // Test invalid versions
    response.set_version(-1);
    EXPECT_FALSE(response.is_valid_version());

    response.set_version(8);
    EXPECT_FALSE(response.is_valid_version());
}

TEST(OffsetFetchResponseTest, EncodeEmptyResponse)
{
    OffsetFetchResponse response;
    response.set_version(0);

    real_encoder encoder(1024);
    EXPECT_EQ(response.encode(encoder), 0);
}

TEST(OffsetFetchResponseTest, EncodeWithBlocks)
{
    OffsetFetchResponse response;
    response.set_version(0);

    // Add a block
    auto block = std::make_shared<OffsetFetchResponseBlock>();
    block->m_offset = 100;
    block->m_metadata = "test-metadata";
    block->m_err = ErrNoError;
    response.add_block("test-topic", 0, block);

    real_encoder encoder(1024);
    EXPECT_EQ(response.encode(encoder), 0);
}

TEST(OffsetFetchResponseTest, EncodeWithVersionSpecificFields)
{
    // Test with version 5 which includes leader epoch
    OffsetFetchResponse response;
    response.set_version(5);

    // Add a block
    auto block = std::make_shared<OffsetFetchResponseBlock>();
    block->m_offset = 100;
    block->m_leader_epoch = 123;
    block->m_metadata = "test-metadata";
    block->m_err = ErrNoError;
    response.add_block("test-topic", 0, block);

    real_encoder encoder(1024);
    EXPECT_EQ(response.encode(encoder), 0);
}