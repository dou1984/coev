#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <memory>

#include "add_offsets_to_txn_request.h"
#include "real_encoder.h"
#include "real_decoder.h"

// Test data from Sarama's add_offsets_to_txn_request_test.go
const unsigned char addOffsetsToTxnRequestData[] = {
    0x00, 0x03, 't', 'x', 'n',                      // transactional_id: "txn"
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x40, // producer_id: 8000 (0x1F40 = 8000)
    0x00, 0x00,                                     // producer_epoch: 0
    0x00, 0x07, 'g', 'r', 'o', 'u', 'p', 'i', 'd'   // group_id: "groupid"
};

const std::string addOffsetsToTxnRequestStr(reinterpret_cast<const char *>(addOffsetsToTxnRequestData), sizeof(addOffsetsToTxnRequestData));

TEST(AddOffsetsToTxnRequestTest, BasicEncodingDecoding)
{
    // Create a simple add offsets to transaction request
    AddOffsetsToTxnRequest request;
    request.set_version(0);
    request.m_transactional_id = "txn";
    request.m_producer_id = 8000;
    request.m_producer_epoch = 0;
    request.m_group_id = "groupid";

    // Test encoding
    real_encoder encoder(1024);
    int encodeResult = request.encode(encoder);
    ASSERT_EQ(encodeResult, 0) << "Failed to encode request";

    const auto &encoded = encoder.m_raw;
    ASSERT_FALSE(encoded.empty()) << "Encoded data is empty";

    // Compare only the actual encoded data, not the entire buffer
    std::string actualEncoded(encoded.data(), encoder.m_offset);
    ASSERT_FALSE(actualEncoded.empty()) << "Actual encoded data is empty";

    // Verify encoded data matches expected values
    EXPECT_EQ(actualEncoded, addOffsetsToTxnRequestStr) << "Encoded data mismatch";

    // Test decoding
    real_decoder decoder;
    decoder.m_raw = actualEncoded;
    decoder.m_offset = 0;

    AddOffsetsToTxnRequest decoded;
    decoded.set_version(0);
    int decodeResult = decoded.decode(decoder, 0);
    ASSERT_EQ(decodeResult, 0) << "Failed to decode request";

    // Verify decoded values
    EXPECT_EQ(decoded.m_transactional_id, "txn") << "Transactional ID mismatch";
    EXPECT_EQ(decoded.m_producer_id, 8000) << "Producer ID mismatch";
    EXPECT_EQ(decoded.m_producer_epoch, 0) << "Producer epoch mismatch";
    EXPECT_EQ(decoded.m_group_id, "groupid") << "Group ID mismatch";
}

TEST(AddOffsetsToTxnRequestTest, VersionProperties)
{
    AddOffsetsToTxnRequest request;

    // Test different versions
    for (int16_t version = 0; version <= 2; ++version)
    {
        request.set_version(version);
        EXPECT_EQ(request.version(), version) << "Version mismatch";
        EXPECT_TRUE(request.is_valid_version()) << "Version " << version << " should be valid";
    }

    // Test invalid version
    request.set_version(10);
    EXPECT_FALSE(request.is_valid_version()) << "Version 10 should be invalid";

    // Test key and header version methods
    request.set_version(0);
    EXPECT_EQ(request.key(), apiKeyAddOffsetsToTxn) << "Request key mismatch";
    EXPECT_EQ(request.header_version(), 1) << "Header version mismatch";
}

TEST(AddOffsetsToTxnRequestTest, RequiredVersion)
{
    AddOffsetsToTxnRequest request;

    // Test required version for different request versions
    request.set_version(0);
    EXPECT_EQ(request.required_version(), V0_11_0_0) << "Required version mismatch for version 0";

    request.set_version(1);
    EXPECT_EQ(request.required_version(), V2_0_0_0) << "Required version mismatch for version 1";

    request.set_version(2);
    EXPECT_EQ(request.required_version(), V2_7_0_0) << "Required version mismatch for version 2";
}

TEST(AddOffsetsToTxnRequestTest, EncodingDifferentVersions)
{
    // Test encoding with different versions
    AddOffsetsToTxnRequest request;
    request.m_transactional_id = "test_txn";
    request.m_producer_id = 12345;
    request.m_producer_epoch = 5;
    request.m_group_id = "test_group";

    for (int16_t version = 0; version <= 2; ++version)
    {
        request.set_version(version);

        real_encoder encoder(1024);
        int encodeResult = request.encode(encoder);
        ASSERT_EQ(encodeResult, 0) << "Failed to encode request with version " << version;

        EXPECT_FALSE(encoder.m_raw.empty()) << "Encoded data should not be empty for version " << version;
    }
}
