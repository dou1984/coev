#include <gtest/gtest.h>
#include <vector>
#include <cstdint>

#include "control_record.h"
#include "real_encoder.h"
#include "real_decoder.h"

// Test data from Sarama's control_record_test.go
const unsigned char abortTxCtrlRecKeyData[] = {0x00, 0x00, 0x00, 0x00}; // version 0, TX_ABORT = 0
const std::string abortTxCtrlRecKey(reinterpret_cast<const char*>(abortTxCtrlRecKeyData), sizeof(abortTxCtrlRecKeyData));

const unsigned char abortTxCtrlRecValueData[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x0A}; // version 0, coordinator epoch = 10
const std::string abortTxCtrlRecValue(reinterpret_cast<const char*>(abortTxCtrlRecValueData), sizeof(abortTxCtrlRecValueData));

const unsigned char commitTxCtrlRecKeyData[] = {0x00, 0x00, 0x00, 0x01}; // version 0, TX_COMMIT = 1
const std::string commitTxCtrlRecKey(reinterpret_cast<const char*>(commitTxCtrlRecKeyData), sizeof(commitTxCtrlRecKeyData));

const unsigned char commitTxCtrlRecValueData[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x0F}; // version 0, coordinator epoch = 15
const std::string commitTxCtrlRecValue(reinterpret_cast<const char*>(commitTxCtrlRecValueData), sizeof(commitTxCtrlRecValueData));

const unsigned char unknownCtrlRecKeyData[] = {0x00, 0x00, 0x00, 0x80}; // version 0, UNKNOWN = -1
const std::string unknownCtrlRecKey(reinterpret_cast<const char*>(unknownCtrlRecKeyData), sizeof(unknownCtrlRecKeyData));

const std::string unknownCtrlRecValue = ""; // empty value for unknown record

TEST(ControlRecordTest, DecodingControlRecords) {
    // Test abort transaction control record
    realDecoder keyDecoder;
    keyDecoder.m_raw = abortTxCtrlRecKey;
    keyDecoder.m_offset = 0;
    
    realDecoder valueDecoder;
    valueDecoder.m_raw = abortTxCtrlRecValue;
    valueDecoder.m_offset = 0;
    
    ControlRecord abortRecord;
    int abortResult = abortRecord.decode(keyDecoder, valueDecoder);
    ASSERT_EQ(abortResult, 0) << "Failed to decode abort control record";
    EXPECT_EQ(abortRecord.m_type, ControlRecordAbort) << "Abort record type mismatch";
    EXPECT_EQ(abortRecord.m_coordinator_epoch, 10) << "Abort coordinator epoch mismatch";
    
    // Test commit transaction control record
    keyDecoder.m_raw = commitTxCtrlRecKey;
    keyDecoder.m_offset = 0;
    
    valueDecoder.m_raw = commitTxCtrlRecValue;
    valueDecoder.m_offset = 0;
    
    ControlRecord commitRecord;
    int commitResult = commitRecord.decode(keyDecoder, valueDecoder);
    ASSERT_EQ(commitResult, 0) << "Failed to decode commit control record";
    EXPECT_EQ(commitRecord.m_type, ControlRecordCommit) << "Commit record type mismatch";
    EXPECT_EQ(commitRecord.m_coordinator_epoch, 15) << "Commit coordinator epoch mismatch";
    
    // Test unknown control record
    keyDecoder.m_raw = unknownCtrlRecKey;
    keyDecoder.m_offset = 0;
    
    valueDecoder.m_raw = unknownCtrlRecValue;
    valueDecoder.m_offset = 0;
    
    ControlRecord unknownRecord;
    int unknownResult = unknownRecord.decode(keyDecoder, valueDecoder);
    ASSERT_EQ(unknownResult, 0) << "Failed to decode unknown control record";
    EXPECT_EQ(unknownRecord.m_type, ControlRecordUnknown) << "Unknown record type mismatch";
}

TEST(ControlRecordTest, EncodingControlRecords) {
    // Test encoding abort transaction control record
    ControlRecord abortRecord(0, 10, ControlRecordAbort);
    
    realEncoder keyEncoder;
    realEncoder valueEncoder;
    
    int abortResult = abortRecord.encode(keyEncoder, valueEncoder);
    ASSERT_EQ(abortResult, 0) << "Failed to encode abort control record";
    
    // Verify encoded data matches expected values
    std::string actualKey(keyEncoder.m_raw.data(), keyEncoder.m_offset);
    std::string actualValue(valueEncoder.m_raw.data(), valueEncoder.m_offset);
    EXPECT_EQ(actualKey, abortTxCtrlRecKey) << "Abort key encoding mismatch";
    EXPECT_EQ(actualValue, abortTxCtrlRecValue) << "Abort value encoding mismatch";
    
    // Test encoding commit transaction control record
    ControlRecord commitRecord(0, 15, ControlRecordCommit);
    
    keyEncoder.m_raw.clear();
    keyEncoder.m_offset = 0;
    valueEncoder.m_raw.clear();
    valueEncoder.m_offset = 0;
    
    int commitResult = commitRecord.encode(keyEncoder, valueEncoder);
    ASSERT_EQ(commitResult, 0) << "Failed to encode commit control record";
    
    // Verify encoded data matches expected values
    std::string actualCommitKey(keyEncoder.m_raw.data(), keyEncoder.m_offset);
    std::string actualCommitValue(valueEncoder.m_raw.data(), valueEncoder.m_offset);
    EXPECT_EQ(actualCommitKey, commitTxCtrlRecKey) << "Commit key encoding mismatch";
    EXPECT_EQ(actualCommitValue, commitTxCtrlRecValue) << "Commit value encoding mismatch";
    
    // Test encoding unknown control record
    ControlRecord unknownRecord(0, 0, ControlRecordUnknown);
    
    keyEncoder.m_raw.clear();
    keyEncoder.m_offset = 0;
    valueEncoder.m_raw.clear();
    valueEncoder.m_offset = 0;
    
    int unknownResult = unknownRecord.encode(keyEncoder, valueEncoder);
    ASSERT_EQ(unknownResult, 0) << "Failed to encode unknown control record";
    
    // Verify encoded key matches expected value
    std::string actualUnknownKey(keyEncoder.m_raw.data(), keyEncoder.m_offset);
    std::string actualUnknownValue(valueEncoder.m_raw.data(), valueEncoder.m_offset);
    EXPECT_EQ(actualUnknownKey, unknownCtrlRecKey) << "Unknown key encoding mismatch";
    // Unknown record should have empty value
    EXPECT_TRUE(actualUnknownValue.empty()) << "Unknown value should be empty";
}

TEST(ControlRecordTest, VersionHandling) {
    // Test that version is properly set and encoded/decoded
    ControlRecord record(1, 20, ControlRecordCommit);
    
    realEncoder keyEncoder;
    realEncoder valueEncoder;
    
    int encodeResult = record.encode(keyEncoder, valueEncoder);
    ASSERT_EQ(encodeResult, 0) << "Failed to encode control record with version 1";
    
    // Decode the encoded record to verify version is preserved
    realDecoder keyDecoder;
    keyDecoder.m_raw = keyEncoder.m_raw;
    keyDecoder.m_offset = 0;
    
    realDecoder valueDecoder;
    valueDecoder.m_raw = valueEncoder.m_raw;
    valueDecoder.m_offset = 0;
    
    ControlRecord decodedRecord;
    int decodeResult = decodedRecord.decode(keyDecoder, valueDecoder);
    ASSERT_EQ(decodeResult, 0) << "Failed to decode control record with version 1";
    
    EXPECT_EQ(decodedRecord.m_version, 1) << "Version not preserved during encode/decode";
    EXPECT_EQ(decodedRecord.m_coordinator_epoch, 20) << "Coordinator epoch not preserved during encode/decode";
    EXPECT_EQ(decodedRecord.m_type, ControlRecordCommit) << "Record type not preserved during encode/decode";
}
