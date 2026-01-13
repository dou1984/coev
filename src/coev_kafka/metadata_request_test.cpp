#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <memory>

#include "metadata_request.h"
#include "real_encoder.h"
#include "real_decoder.h"

// Simplified test to focus on basic functionality
TEST(MetadataRequestTest, BasicEncodingDecoding)
{
    // Create a simple metadata request
    MetadataRequest request;
    request.set_version(0);
    request.m_topics = {"topic1"};
    
    // Test encoding
    realEncoder encoder;
    int encodeResult = request.encode(encoder);
    ASSERT_EQ(encodeResult, 0) << "Failed to encode request";
    
    const auto& encoded = encoder.m_raw;
    ASSERT_FALSE(encoded.empty()) << "Encoded data is empty";
    
    // Print encoded data for debugging
    std::cout << "Encoded data size: " << encoded.size() << std::endl;
    std::cout << "Encoded data: ";
    for (unsigned char c : encoded) {
        std::cout << std::hex << static_cast<int>(c) << " ";
    }
    std::cout << std::dec << std::endl;
    
    // Test decoding
    realDecoder decoder;
    decoder.m_raw = encoded;
    decoder.m_offset = 0;
    
    MetadataRequest decoded;
    decoded.set_version(0);
    int decodeResult = decoded.decode(decoder, 0);
    ASSERT_EQ(decodeResult, 0) << "Failed to decode request";
    
    // Verify decoded values
    ASSERT_EQ(decoded.m_topics.size(), 1) << "Topics count mismatch";
    EXPECT_EQ(decoded.m_topics[0], "topic1") << "Topic name mismatch";
}

// Test factory method
TEST(MetadataRequestTest, FactoryMethod)
{
    auto request = NewMetadataRequest(V0_10_2_0, {"topic1", "topic2"});
    EXPECT_NE(request, nullptr);
    EXPECT_EQ(request->m_topics.size(), 2);
    EXPECT_EQ(request->m_topics[0], "topic1");
    EXPECT_EQ(request->m_topics[1], "topic2");
}

// Test version properties
TEST(MetadataRequestTest, VersionProperties)
{
    MetadataRequest request;
    
    // Check for non-flexible version
    request.set_version(8);
    EXPECT_FALSE(request.is_flexible());
    
    // Check for flexible version
    request.set_version(9);
    EXPECT_TRUE(request.is_flexible());
    
    // Check key and version methods
    request.set_version(5);
    EXPECT_EQ(request.key(), 3); // MetadataRequest key is 3
    EXPECT_EQ(request.version(), 5);
}

