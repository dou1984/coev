#include <gtest/gtest.h>
#include "real_encoder.h"
#include "real_decoder.h"

TEST(EncoderDecoderTest, TestInt8) {
    int8_t test_value = 42;
    int8_t result_value = 0;
    
    real_encoder encoder(100);
    encoder.putInt8(test_value);
    
    real_decoder decoder;
    std::string encodedData = encoder.m_raw.substr(0, encoder.m_offset);
    decoder.m_raw = encodedData;
    decoder.m_offset = 0;
    
    int err = decoder.getInt8(result_value);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(result_value, test_value);
}

TEST(EncoderDecoderTest, TestInt16) {
    int16_t test_value = 32767;
    int16_t result_value = 0;
    
    real_encoder encoder(100);
    encoder.putInt16(test_value);
    
    real_decoder decoder;
    std::string encodedData(encoder.m_raw.data(), encoder.m_offset);
    decoder.m_raw = encodedData;
    decoder.m_offset = 0;
    
    int err = decoder.getInt16(result_value);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(result_value, test_value);
}

TEST(EncoderDecoderTest, TestInt32) {
    int32_t test_value = 2147483647;
    int32_t result_value = 0;
    
    real_encoder encoder(100);
    encoder.putInt32(test_value);
    
    real_decoder decoder;
    std::string encodedData = encoder.m_raw.substr(0, encoder.m_offset);
    decoder.m_raw = encodedData;
    decoder.m_offset = 0;
    
    int err = decoder.getInt32(result_value);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(result_value, test_value);
}

TEST(EncoderDecoderTest, TestInt64) {
    int64_t test_value = 9223372036854775807LL;
    int64_t result_value = 0;
    
    real_encoder encoder(100);
    encoder.putInt64(test_value);
    
    real_decoder decoder;
    std::string encodedData(encoder.m_raw.data(), encoder.m_offset);
    decoder.m_raw = encodedData;
    decoder.m_offset = 0;
    
    int err = decoder.getInt64(result_value);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(result_value, test_value);
}

TEST(EncoderDecoderTest, TestVariant) {
    int64_t test_value = 123456789;
    int64_t result_value = 0;
    
    real_encoder encoder(100);
    encoder.putVariant(test_value);
    
    real_decoder decoder;
    decoder.m_raw = encoder.m_raw.substr(0, encoder.m_offset);
    decoder.m_offset = 0;
    
    int err = decoder.getVariant(result_value);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(result_value, test_value);
}

TEST(EncoderDecoderTest, TestUVarint) {
    uint64_t test_value = 9876543210ULL;
    uint64_t result_value = 0;
    
    real_encoder encoder(100);
    encoder.putUVarint(test_value);
    
    real_decoder decoder;
    decoder.m_raw = encoder.m_raw.substr(0, encoder.m_offset);
    decoder.m_offset = 0;
    
    int err = decoder.getUVariant(result_value);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(result_value, test_value);
}

TEST(EncoderDecoderTest, TestFloat64) {
    double test_value = 3.141592653589793;
    double result_value = 0.0;
    
    real_encoder encoder(100);
    encoder.putFloat64(test_value);
    
    real_decoder decoder;
    decoder.m_raw = encoder.m_raw.substr(0, encoder.m_offset);
    decoder.m_offset = 0;
    
    int err = decoder.getFloat64(result_value);
    EXPECT_EQ(err, 0);
    EXPECT_DOUBLE_EQ(result_value, test_value);
}

TEST(EncoderDecoderTest, TestBool) {
    bool test_value = true;
    bool result_value = false;
    
    real_encoder encoder(100);
    encoder.putBool(test_value);
    
    real_decoder decoder;
    decoder.m_raw = encoder.m_raw.substr(0, encoder.m_offset);
    decoder.m_offset = 0;
    
    int err = decoder.getBool(result_value);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(result_value, test_value);
}

TEST(EncoderDecoderTest, TestString) {
    std::string test_value = "Hello, Kafka!";
    std::string result_value;
    
    real_encoder encoder(100);
    encoder.putString(test_value);
    
    real_decoder decoder;
    decoder.m_raw = encoder.m_raw.substr(0, encoder.m_offset);
    decoder.m_offset = 0;
    
    int err = decoder.getString(result_value);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(result_value, test_value);
}

TEST(EncoderDecoderTest, TestNullableString) {
    std::string test_value = "Nullable string test";
    std::string result_value;
    
    real_encoder encoder(100);
    encoder.putNullableString(test_value);
    
    real_decoder decoder;
    decoder.m_raw = encoder.m_raw.substr(0, encoder.m_offset);
    decoder.m_offset = 0;
    
    int err = decoder.getNullableString(result_value);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(result_value, test_value);
}

TEST(EncoderDecoderTest, TestBytes) {
    std::string test_value = "Raw bytes test";
    std::string result_value;
    
    real_encoder encoder(100);
    encoder.putBytes(test_value);
    
    real_decoder decoder;
    decoder.m_raw = encoder.m_raw.substr(0, encoder.m_offset);
    decoder.m_offset = 0;
    
    int err = decoder.getBytes(result_value);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(result_value, test_value);
}

TEST(EncoderDecoderTest, TestStringArray) {
    std::vector<std::string> test_value = {"item1", "item2", "item3"};
    std::vector<std::string> result_value;
    
    real_encoder encoder(100);
    encoder.putStringArray(test_value);
    
    real_decoder decoder;
    decoder.m_raw = encoder.m_raw.substr(0, encoder.m_offset);
    decoder.m_offset = 0;
    
    int err = decoder.getStringArray(result_value);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(result_value.size(), test_value.size());
    for (size_t i = 0; i < test_value.size(); ++i) {
        EXPECT_EQ(result_value[i], test_value[i]);
    }
}

TEST(EncoderDecoderTest, TestInt32Array) {
    std::vector<int32_t> test_value = {1, 2, 3, 4, 5};
    std::vector<int32_t> result_value;
    
    real_encoder encoder(100);
    encoder.putInt32Array(test_value);
    
    real_decoder decoder;
    decoder.m_raw = encoder.m_raw.substr(0, encoder.m_offset);
    decoder.m_offset = 0;
    
    int err = decoder.getInt32Array(result_value);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(result_value.size(), test_value.size());
    for (size_t i = 0; i < test_value.size(); ++i) {
        EXPECT_EQ(result_value[i], test_value[i]);
    }
}

TEST(EncoderDecoderTest, TestInt64Array) {
    std::vector<int64_t> test_value = {1000, 2000, 3000, 4000, 5000};
    std::vector<int64_t> result_value;
    
    real_encoder encoder(100);
    encoder.putInt64Array(test_value);
    
    real_decoder decoder;
    decoder.m_raw = encoder.m_raw.substr(0, encoder.m_offset);
    decoder.m_offset = 0;
    
    int err = decoder.getInt64Array(result_value);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(result_value.size(), test_value.size());
    for (size_t i = 0; i < test_value.size(); ++i) {
        EXPECT_EQ(result_value[i], test_value[i]);
    }
}


