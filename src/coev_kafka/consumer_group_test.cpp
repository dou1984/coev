#include <gtest/gtest.h>
#include "join_group_request.h"
#include "join_group_response.h"
#include "sync_group_request.h"
#include "sync_group_response.h"
#include "heartbeat_request.h"
#include "heartbeat_response.h"
#include "leave_group_request.h"
#include "leave_group_response.h"

TEST(ConsumerGroupTest, JoinGroupRequestVersionCompatibility) {
    JoinGroupRequest request;
    
    // Test different versions (use more conservative range)
    for (int16_t version = 0; version <= 4; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(ConsumerGroupTest, JoinGroupResponseVersionCompatibility) {
    JoinGroupResponse response;
    
    // Test different versions (use more conservative range)
    for (int16_t version = 0; version <= 4; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(ConsumerGroupTest, SyncGroupRequestVersionCompatibility) {
    SyncGroupRequest request;
    
    // Test different versions
    for (int16_t version = 0; version <= 4; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(ConsumerGroupTest, SyncGroupResponseVersionCompatibility) {
    SyncGroupResponse response;
    
    // Test different versions
    for (int16_t version = 0; version <= 4; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(ConsumerGroupTest, HeartbeatRequestVersionCompatibility) {
    HeartbeatRequest request;
    
    // Test different versions
    for (int16_t version = 0; version <= 4; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(ConsumerGroupTest, HeartbeatResponseVersionCompatibility) {
    HeartbeatResponse response;
    
    // Test different versions
    for (int16_t version = 0; version <= 4; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(ConsumerGroupTest, LeaveGroupRequestVersionCompatibility) {
    LeaveGroupRequest request;
    
    // Test different versions
    for (int16_t version = 0; version <= 4; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(ConsumerGroupTest, LeaveGroupResponseVersionCompatibility) {
    LeaveGroupResponse response;
    
    // Test different versions
    for (int16_t version = 0; version <= 4; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(ConsumerGroupTest, BasicFunctionality) {
    JoinGroupRequest join_request;
    join_request.set_version(2);
    EXPECT_EQ(2, join_request.version());
    
    JoinGroupResponse join_response;
    join_response.set_version(2);
    EXPECT_EQ(2, join_response.version());
    
    SyncGroupRequest sync_request;
    sync_request.set_version(2);
    EXPECT_EQ(2, sync_request.version());
    
    SyncGroupResponse sync_response;
    sync_response.set_version(2);
    EXPECT_EQ(2, sync_response.version());
    
    HeartbeatRequest heartbeat_request;
    heartbeat_request.set_version(2);
    EXPECT_EQ(2, heartbeat_request.version());
    
    HeartbeatResponse heartbeat_response;
    heartbeat_response.set_version(2);
    EXPECT_EQ(2, heartbeat_response.version());
    
    LeaveGroupRequest leave_request;
    leave_request.set_version(2);
    EXPECT_EQ(2, leave_request.version());
    
    LeaveGroupResponse leave_response;
    leave_response.set_version(2);
    EXPECT_EQ(2, leave_response.version());
}