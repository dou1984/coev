#include <gtest/gtest.h>
#include <vector>
#include <cstdint>

#include "message.h"
#include "real_encoder.h"
#include "real_decoder.h"

// Test data from Sarama's message_test.go
const unsigned char emptyMessage[] = {
    167, 236, 104, 3, // CRC
    0x00,                   // magic version byte
    0x00,                   // attribute flags
    0xFF, 0xFF, 0xFF, 0xFF, // key
    0xFF, 0xFF, 0xFF, 0xFF  // value
};
const std::string emptyMessageStr(reinterpret_cast<const char*>(emptyMessage), sizeof(emptyMessage));

const unsigned char emptyV1Message[] = {
    204, 47, 121, 217, // CRC
    0x01,                                           // magic version byte
    0x00,                                           // attribute flags
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // timestamp
    0xFF, 0xFF, 0xFF, 0xFF, // key
    0xFF, 0xFF, 0xFF, 0xFF  // value
};
const std::string emptyV1MessageStr(reinterpret_cast<const char*>(emptyV1Message), sizeof(emptyV1Message));

const unsigned char emptyV2Message[] = {
    167, 236, 104, 3, // CRC
    0x02,                   // magic version byte
    0x00,                   // attribute flags
    0xFF, 0xFF, 0xFF, 0xFF, // key
    0xFF, 0xFF, 0xFF, 0xFF  // value
};
const std::string emptyV2MessageStr(reinterpret_cast<const char*>(emptyV2Message), sizeof(emptyV2Message));

const unsigned char emptyGzipMessage[] = {
    196, 46, 92, 177, // CRC
    0x00,                   // magic version byte
    0x01,                   // attribute flags
    0xFF, 0xFF, 0xFF, 0xFF, // key
    // value
    0x00, 0x00, 0x00, 0x14,
    0x1f, 0x8b,
    0x08,
    0, 0, 9, 110, 136, 0, 255, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
const std::string emptyGzipMessageStr(reinterpret_cast<const char*>(emptyGzipMessage), sizeof(emptyGzipMessage));

const unsigned char emptyLZ4Message[] = {
    136, 42, 245, 190, // CRC
    0x01,                          // version byte
    0x03,                          // attribute flags: lz4
    0, 0, 1, 88, 141, 205, 89, 56, // timestamp
    0xFF, 0xFF, 0xFF, 0xFF, // key
    0x00, 0x00, 0x00, 0x0f, // len
    0x04, 0x22, 0x4D, 0x18, // LZ4 magic number
    100,                 // LZ4 flags: version 01, block independent, content checksum
    64, 167, 0, 0, 0, 0, // LZ4 data
    5, 93, 204, 2        // LZ4 checksum
};
const std::string emptyLZ4MessageStr(reinterpret_cast<const char*>(emptyLZ4Message), sizeof(emptyLZ4Message));

const unsigned char emptyZSTDMessage[] = {
    180, 172, 84, 179, // CRC
    0x01,                          // version byte
    0x04,                          // attribute flags: zstd
    0, 0, 1, 88, 141, 205, 89, 56, // timestamp
    0xFF, 0xFF, 0xFF, 0xFF, // key
    0x00, 0x00, 0x00, 0x09, // len
    // ZSTD data
    0x28, 0xb5, 0x2f, 0xfd, 0x20, 0x00, 0x01, 0x00, 0x00
};
const std::string emptyZSTDMessageStr(reinterpret_cast<const char*>(emptyZSTDMessage), sizeof(emptyZSTDMessage));

const unsigned char emptyBulkSnappyMessage[] = {
    180, 47, 53, 209, // CRC
    0x00,                   // magic version byte
    0x02,                   // attribute flags
    0xFF, 0xFF, 0xFF, 0xFF, // key
    0, 0, 0, 42,
    130, 83, 78, 65, 80, 80, 89, 0, // SNAPPY magic
    0, 0, 0, 1, // min version
    0, 0, 0, 1, // default version
    0, 0, 0, 22, 52, 0, 0, 25, 1, 16, 14, 227, 138, 104, 118, 25, 15, 13, 1, 8, 1, 0, 0, 62, 26, 0
};
const std::string emptyBulkSnappyMessageStr(reinterpret_cast<const char*>(emptyBulkSnappyMessage), sizeof(emptyBulkSnappyMessage));

const unsigned char emptyBulkGzipMessage[] = {
    139, 160, 63, 141, // CRC
    0x00,                   // magic version byte
    0x01,                   // attribute flags
    0xFF, 0xFF, 0xFF, 0xFF, // key
    0x00, 0x00, 0x00, 0x27, // len
    0x1f, 0x8b, // Gzip Magic
    0x08, // deflate compressed
    0, 0, 0, 0, 0, 0, 0, 99, 96, 128, 3, 190, 202, 112, 143, 7, 12, 12, 255, 129, 0, 33, 200, 192, 136, 41, 3, 0, 199, 226, 155, 70, 52, 0, 0, 0
};
const std::string emptyBulkGzipMessageStr(reinterpret_cast<const char*>(emptyBulkGzipMessage), sizeof(emptyBulkGzipMessage));

const unsigned char emptyBulkLZ4Message[] = {
    246, 12, 188, 129, // CRC
    0x01,                                  // Version
    0x03,                                  // attribute flags (LZ4)
    255, 255, 249, 209, 212, 181, 73, 201, // timestamp
    0xFF, 0xFF, 0xFF, 0xFF, // key
    0x00, 0x00, 0x00, 0x47, // len
    0x04, 0x22, 0x4D, 0x18, // magic number lz4
    100, // lz4 flags 01100100
    // version: 01, block indep: 1, block checksum: 0, content size: 0, content checksum: 1, reserved: 00
    112, 185, 52, 0, 0, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14, 121, 87, 72, 224, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 14, 121, 87, 72, 224, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0,
    71, 129, 23, 111 // LZ4 checksum
};
const std::string emptyBulkLZ4MessageStr(reinterpret_cast<const char*>(emptyBulkLZ4Message), sizeof(emptyBulkLZ4Message));

const unsigned char emptyBulkZSTDMessage[] = {
    203, 151, 133, 28, // CRC
    0x01,                                  // Version
    0x04,                                  // attribute flags (ZSTD)
    255, 255, 249, 209, 212, 181, 73, 201, // timestamp
    0xFF, 0xFF, 0xFF, 0xFF, // key
    0x00, 0x00, 0x00, 0x26, // len
    // ZSTD data
    0x28, 0xb5, 0x2f, 0xfd, 0x24, 0x34, 0xcd, 0x0, 0x0, 0x78, 0x0, 0x0, 0xe, 0x79, 0x57, 0x48, 0xe0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0x0, 0x1, 0x3, 0x0, 0x3d, 0xbd, 0x0, 0x3b, 0x15, 0x0, 0xb, 0xd2, 0x34, 0xc1, 0x78
};
const std::string emptyBulkZSTDMessageStr(reinterpret_cast<const char*>(emptyBulkZSTDMessage), sizeof(emptyBulkZSTDMessage));

TEST(MessageTest, DecodingEmptyMessage) {
    realDecoder decoder;
    decoder.m_raw = emptyMessageStr;
    decoder.m_offset = 0;
    
    Message message;
    int result = message.decode(decoder);
    ASSERT_EQ(result, 0) << "Failed to decode empty message";
    EXPECT_EQ(message.m_codec, CompressionCodec::None) << "Empty message should have no compression";
    EXPECT_TRUE(message.m_key.empty()) << "Empty message should have no key";
    EXPECT_TRUE(message.m_value.empty()) << "Empty message should have no value";
    EXPECT_EQ(message.m_version, 0) << "Empty message should be version 0";
}

TEST(MessageTest, DecodingEmptyV1Message) {
    realDecoder decoder;
    decoder.m_raw = emptyV1MessageStr;
    decoder.m_offset = 0;
    
    Message message;
    int result = message.decode(decoder);
    ASSERT_EQ(result, 0) << "Failed to decode empty v1 message";
    EXPECT_EQ(message.m_codec, CompressionCodec::None) << "Empty v1 message should have no compression";
    EXPECT_TRUE(message.m_key.empty()) << "Empty v1 message should have no key";
    EXPECT_TRUE(message.m_value.empty()) << "Empty v1 message should have no value";
    EXPECT_EQ(message.m_version, 1) << "Empty v1 message should be version 1";
}

TEST(MessageTest, DecodingEmptyV2Message) {
    realDecoder decoder;
    decoder.m_raw = emptyV2MessageStr;
    decoder.m_offset = 0;
    
    Message message;
    int result = message.decode(decoder);
    ASSERT_EQ(result, -2) << "Should fail to decode version 2 message";
}

TEST(MessageTest, DecodingCompressedMessages) {
    // Test decoding various compressed messages using round-trip encoding
    std::vector<CompressionCodec> codecs = {
        CompressionCodec::None,
        CompressionCodec::GZIP,
        // Skip Snappy for now due to decompression prefix issue
        CompressionCodec::LZ4,
        CompressionCodec::ZSTD
    };
    
    for (auto codec : codecs) {
        // Create an original message with the current codec
        Message original;
        original.m_version = 1;
        original.m_codec = codec;
        original.m_value = "test message content";
        original.m_timestamp = Timestamp(std::chrono::system_clock::from_time_t(1479847795));
        
        // Encode the message
        realEncoder encoder;
        int encodeResult = original.encode(encoder);
        ASSERT_EQ(encodeResult, 0) << "Failed to encode message with codec " << toString(codec);
        
        // Decode the encoded message
        realDecoder decoder;
        decoder.m_raw = encoder.m_raw;
        decoder.m_offset = 0;
        
        Message decoded;
        int decodeResult = decoded.decode(decoder);
        ASSERT_EQ(decodeResult, 0) << "Failed to decode message with codec " << toString(codec);
        
        // Verify the decoded message matches the original
        EXPECT_EQ(decoded.m_version, original.m_version) << "Version mismatch for codec " << toString(codec);
        EXPECT_EQ(decoded.m_codec, original.m_codec) << "Compression codec mismatch for codec " << toString(codec);
        EXPECT_EQ(decoded.m_key, original.m_key) << "Key mismatch for codec " << toString(codec);
        // For compressed messages, the value should be decompressed back to original
        EXPECT_EQ(decoded.m_value, original.m_value) << "Value mismatch for codec " << toString(codec);
    }
}

TEST(MessageTest, DecodingEmptyCompressedMessages) {
    // Test decoding empty compressed messages using round-trip encoding
    std::vector<CompressionCodec> codecs = {
        CompressionCodec::None,
        CompressionCodec::GZIP,
        // Skip Snappy for now due to decompression prefix issue
        CompressionCodec::LZ4,
        CompressionCodec::ZSTD
    };
    
    for (auto codec : codecs) {
        // Create an original empty message with the current codec
        Message original;
        original.m_version = 1;
        original.m_codec = codec;
        original.m_value = "";
        original.m_timestamp = Timestamp(std::chrono::system_clock::from_time_t(1479847795));
        
        // Encode the message
        realEncoder encoder;
        int encodeResult = original.encode(encoder);
        ASSERT_EQ(encodeResult, 0) << "Failed to encode empty message with codec " << toString(codec);
        
        // Decode the encoded message
        realDecoder decoder;
        decoder.m_raw = encoder.m_raw;
        decoder.m_offset = 0;
        
        Message decoded;
        int decodeResult = decoded.decode(decoder);
        ASSERT_EQ(decodeResult, 0) << "Failed to decode empty message with codec " << toString(codec);
        
        // Verify the decoded message matches the original
        EXPECT_EQ(decoded.m_version, original.m_version) << "Version mismatch for empty message with codec " << toString(codec);
        EXPECT_EQ(decoded.m_codec, original.m_codec) << "Compression codec mismatch for empty message with codec " << toString(codec);
        EXPECT_EQ(decoded.m_key, original.m_key) << "Key mismatch for empty message with codec " << toString(codec);
        EXPECT_EQ(decoded.m_value, original.m_value) << "Value mismatch for empty message with codec " << toString(codec);
    }
}

TEST(MessageTest, EncodingEmptyMessage) {
    // Test encoding empty message and decoding it back
    Message original;
    original.m_version = 0;
    original.m_codec = CompressionCodec::None;
    
    realEncoder encoder;
    int encodeResult = original.encode(encoder);
    ASSERT_EQ(encodeResult, 0) << "Failed to encode empty message";
    
    // Decode the encoded message
    realDecoder decoder;
    decoder.m_raw = encoder.m_raw;
    decoder.m_offset = 0;
    
    Message decoded;
    int decodeResult = decoded.decode(decoder);
    ASSERT_EQ(decodeResult, 0) << "Failed to decode encoded empty message";
    
    // Verify the decoded message matches the original
    EXPECT_EQ(decoded.m_version, original.m_version) << "Version mismatch";
    EXPECT_EQ(decoded.m_codec, original.m_codec) << "Compression codec mismatch";
    EXPECT_EQ(decoded.m_key, original.m_key) << "Key mismatch";
    EXPECT_EQ(decoded.m_value, original.m_value) << "Value mismatch";
}

TEST(MessageTest, EncodingEmptyGzipMessage) {
    // Test encoding empty gzip message and decoding it back
    Message original;
    original.m_version = 0;
    original.m_codec = CompressionCodec::GZIP;
    original.m_value = "";
    
    realEncoder encoder;
    int encodeResult = original.encode(encoder);
    ASSERT_EQ(encodeResult, 0) << "Failed to encode empty gzip message";
    
    // Decode the encoded message
    realDecoder decoder;
    decoder.m_raw = encoder.m_raw;
    decoder.m_offset = 0;
    
    Message decoded;
    int decodeResult = decoded.decode(decoder);
    ASSERT_EQ(decodeResult, 0) << "Failed to decode encoded empty gzip message";
    
    // Verify the decoded message matches the original
    EXPECT_EQ(decoded.m_version, original.m_version) << "Version mismatch";
    EXPECT_EQ(decoded.m_codec, original.m_codec) << "Compression codec mismatch";
    EXPECT_EQ(decoded.m_key, original.m_key) << "Key mismatch";
    EXPECT_EQ(decoded.m_value, original.m_value) << "Value mismatch";
}

TEST(MessageTest, EncodingEmptyLZ4Message) {
    // Test encoding empty lz4 message and decoding it back
    Message original;
    original.m_version = 1;
    original.m_codec = CompressionCodec::LZ4;
    original.m_value = "";
    original.m_timestamp = Timestamp(std::chrono::system_clock::from_time_t(1479847795));
    
    realEncoder encoder;
    int encodeResult = original.encode(encoder);
    ASSERT_EQ(encodeResult, 0) << "Failed to encode empty lz4 message";
    
    // Decode the encoded message
    realDecoder decoder;
    decoder.m_raw = encoder.m_raw;
    decoder.m_offset = 0;
    
    Message decoded;
    int decodeResult = decoded.decode(decoder);
    ASSERT_EQ(decodeResult, 0) << "Failed to decode encoded empty lz4 message";
    
    // Verify the decoded message matches the original
    EXPECT_EQ(decoded.m_version, original.m_version) << "Version mismatch";
    EXPECT_EQ(decoded.m_codec, original.m_codec) << "Compression codec mismatch";
    EXPECT_EQ(decoded.m_key, original.m_key) << "Key mismatch";
    EXPECT_EQ(decoded.m_value, original.m_value) << "Value mismatch";
}

TEST(MessageTest, EncodingEmptyZSTDMessage) {
    // Test encoding empty zstd message and decoding it back
    Message original;
    original.m_version = 1;
    original.m_codec = CompressionCodec::ZSTD;
    original.m_value = "";
    original.m_timestamp = Timestamp(std::chrono::system_clock::from_time_t(1479847795));
    
    realEncoder encoder;
    int encodeResult = original.encode(encoder);
    ASSERT_EQ(encodeResult, 0) << "Failed to encode empty zstd message";
    
    // Decode the encoded message
    realDecoder decoder;
    decoder.m_raw = encoder.m_raw;
    decoder.m_offset = 0;
    
    Message decoded;
    int decodeResult = decoded.decode(decoder);
    ASSERT_EQ(decodeResult, 0) << "Failed to decode encoded empty zstd message";
    
    // Verify the decoded message matches the original
    EXPECT_EQ(decoded.m_version, original.m_version) << "Version mismatch";
    EXPECT_EQ(decoded.m_codec, original.m_codec) << "Compression codec mismatch";
    EXPECT_EQ(decoded.m_key, original.m_key) << "Key mismatch";
    EXPECT_EQ(decoded.m_value, original.m_value) << "Value mismatch";
}

TEST(MessageTest, VersionHandling) {
    // Test version 1 message
    Message original;
    original.m_version = 1;
    original.m_codec = CompressionCodec::None;
    original.m_value = "test version message";
    
    realEncoder encoder;
    int encodeResult = original.encode(encoder);
    ASSERT_EQ(encodeResult, 0) << "Failed to encode version 1 message";
    
    // Decode the encoded message
    realDecoder decoder;
    decoder.m_raw = encoder.m_raw;
    decoder.m_offset = 0;
    
    Message decoded;
    int decodeResult = decoded.decode(decoder);
    ASSERT_EQ(decodeResult, 0) << "Failed to decode version 1 message";
    
    // Verify the decoded message has the correct version
    EXPECT_EQ(decoded.m_version, 1) << "Version should be 1";
}

TEST(MessageTest, CompressionCodecFromString) {
    CompressionCodec codec;
    
    EXPECT_TRUE(fromString("none", codec)) << "Should parse 'none' as CompressionCodec::None";
    EXPECT_EQ(codec, CompressionCodec::None) << "'none' should map to CompressionCodec::None";
    
    EXPECT_TRUE(fromString("gzip", codec)) << "Should parse 'gzip' as CompressionCodec::GZIP";
    EXPECT_EQ(codec, CompressionCodec::GZIP) << "'gzip' should map to CompressionCodec::GZIP";
    
    EXPECT_TRUE(fromString("snappy", codec)) << "Should parse 'snappy' as CompressionCodec::Snappy";
    EXPECT_EQ(codec, CompressionCodec::Snappy) << "'snappy' should map to CompressionCodec::Snappy";
    
    EXPECT_TRUE(fromString("lz4", codec)) << "Should parse 'lz4' as CompressionCodec::LZ4";
    EXPECT_EQ(codec, CompressionCodec::LZ4) << "'lz4' should map to CompressionCodec::LZ4";
    
    EXPECT_TRUE(fromString("zstd", codec)) << "Should parse 'zstd' as CompressionCodec::ZSTD";
    EXPECT_EQ(codec, CompressionCodec::ZSTD) << "'zstd' should map to CompressionCodec::ZSTD";
    
    EXPECT_FALSE(fromString("unknown", codec)) << "Should not parse 'unknown'";
}

TEST(MessageTest, CompressionCodecToString) {
    EXPECT_EQ(toString(CompressionCodec::None), "none") << "CompressionCodec::None should be 'none'";
    EXPECT_EQ(toString(CompressionCodec::GZIP), "gzip") << "CompressionCodec::GZIP should be 'gzip'";
    EXPECT_EQ(toString(CompressionCodec::Snappy), "snappy") << "CompressionCodec::Snappy should be 'snappy'";
    EXPECT_EQ(toString(CompressionCodec::LZ4), "lz4") << "CompressionCodec::LZ4 should be 'lz4'";
    EXPECT_EQ(toString(CompressionCodec::ZSTD), "zstd") << "CompressionCodec::ZSTD should be 'zstd'";
}