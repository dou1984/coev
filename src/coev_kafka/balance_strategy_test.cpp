#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include "balance_strategy.h"
#include "consumer_group_members.h"
#include "sticky_assignor_user_data.h"

// Test fixture for BalanceStrategy tests
class BalanceStrategyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup common test resources
        // Create sample topics and partitions
        testTopics["topic1"] = {0, 1, 2, 3, 4};
        testTopics["topic2"] = {0, 1, 2};
        testTopics["topic3"] = {0, 1};
        
        // Create sample members
        testMembers["member1"].m_version = 0;
        testMembers["member1"].m_topics = {"topic1", "topic2"};
        
        testMembers["member2"].m_version = 0;
        testMembers["member2"].m_topics = {"topic1", "topic2", "topic3"};
        
        testMembers["member3"].m_version = 0;
        testMembers["member3"].m_topics = {"topic2", "topic3"};
    }
    
    void TearDown() override {
        // Cleanup test resources
    }
    
    // Helper function to create ConsumerGroupMemberMetadata
    ConsumerGroupMemberMetadata createMemberMetadata(const std::vector<std::string>& topics) {
        ConsumerGroupMemberMetadata metadata;
        metadata.m_version = 0;
        metadata.m_topics = topics;
        return metadata;
    }
    
    // Common test data
    std::map<std::string, std::vector<int32_t>> testTopics;
    std::map<std::string, ConsumerGroupMemberMetadata> testMembers;
};

// Test Range Balance Strategy
TEST_F(BalanceStrategyTest, RangeStrategyName) {
    auto strategy = NewBalanceStrategyRange();
    EXPECT_EQ(strategy->Name(), "range") << "Range strategy should return correct name";
}

TEST_F(BalanceStrategyTest, RangeStrategyPlan) {
    auto strategy = NewBalanceStrategyRange();
    BalanceStrategyPlan plan;
    
    int result = strategy->Plan(testMembers, testTopics, plan);
    EXPECT_EQ(result, 0) << "Range strategy Plan should succeed";
    EXPECT_FALSE(plan.empty()) << "Range strategy should generate non-empty plan";
}

TEST_F(BalanceStrategyTest, RangeStrategyAssignmentData) {
    auto strategy = NewBalanceStrategyRange();
    std::string data;
    
    std::map<std::string, std::vector<int32_t>> memberTopics = {
        {"topic1", {0, 1, 2}},
        {"topic2", {0, 1}}
    };
    
    int result = strategy->AssignmentData("test-member", memberTopics, 1, data);
    EXPECT_EQ(result, 0) << "Range strategy AssignmentData should succeed";
}

// Test Range Balance Strategy with specific scenarios from Sarama
TEST_F(BalanceStrategyTest, RangeStrategyPlanScenarios) {
    auto strategy = NewBalanceStrategyRange();
    
    // Scenario 1: 2 members, 2 topics, 4 partitions each
    {
        std::map<std::string, ConsumerGroupMemberMetadata> members = {
            {"M1", createMemberMetadata({"T1", "T2"})},
            {"M2", createMemberMetadata({"T1", "T2"})}
        };
        
        std::map<std::string, std::vector<int32_t>> topics = {
            {"T1", {0, 1, 2, 3}},
            {"T2", {0, 1, 2, 3}}
        };
        
        BalanceStrategyPlan plan;
        EXPECT_EQ(strategy->Plan(members, topics, plan), 0) << "Range plan should succeed for scenario 1";
        EXPECT_FALSE(plan.empty()) << "Range plan should not be empty for scenario 1";
    }
    
    // Scenario 2: 2 members, 2 topics, different subscriptions
    {
        std::map<std::string, ConsumerGroupMemberMetadata> members = {
            {"M1", createMemberMetadata({"T1"})},
            {"M2", createMemberMetadata({"T1", "T2"})}
        };
        
        std::map<std::string, std::vector<int32_t>> topics = {
            {"T1", {0, 1}},
            {"T2", {0, 1}}
        };
        
        BalanceStrategyPlan plan;
        EXPECT_EQ(strategy->Plan(members, topics, plan), 0) << "Range plan should succeed for scenario 2";
        EXPECT_FALSE(plan.empty()) << "Range plan should not be empty for scenario 2";
    }
}

// Test Sticky Balance Strategy
TEST_F(BalanceStrategyTest, StickyStrategyName) {
    auto strategy = NewBalanceStrategySticky();
    EXPECT_EQ(strategy->Name(), "sticky") << "Sticky strategy should return correct name";
}

TEST_F(BalanceStrategyTest, StickyStrategyPlan) {
    auto strategy = NewBalanceStrategySticky();
    BalanceStrategyPlan plan;
    
    int result = strategy->Plan(testMembers, testTopics, plan);
    EXPECT_EQ(result, 0) << "Sticky strategy Plan should succeed";
    EXPECT_FALSE(plan.empty()) << "Sticky strategy should generate non-empty plan";
}

TEST_F(BalanceStrategyTest, StickyStrategyAssignmentData) {
    auto strategy = NewBalanceStrategySticky();
    std::string data;
    
    std::map<std::string, std::vector<int32_t>> memberTopics = {
        {"topic1", {0, 1}},
        {"topic3", {0}}
    };
    
    int result = strategy->AssignmentData("test-member", memberTopics, 2, data);
    EXPECT_EQ(result, 0) << "Sticky strategy AssignmentData should succeed";
}

// Test Sticky Balance Strategy with specific scenarios from Sarama
TEST_F(BalanceStrategyTest, StickyStrategyPlanScenarios) {
    auto strategy = NewBalanceStrategySticky();
    
    // Scenario 1: One consumer with no topics
    {
        std::map<std::string, ConsumerGroupMemberMetadata> members = {
            {"consumer", createMemberMetadata({})}
        };
        
        std::map<std::string, std::vector<int32_t>> topics = {};
        
        BalanceStrategyPlan plan;
        EXPECT_EQ(strategy->Plan(members, topics, plan), 0) << "Sticky plan should succeed for scenario 1";
    }
    
    // Scenario 2: One consumer with one topic
    {
        std::map<std::string, ConsumerGroupMemberMetadata> members = {
            {"consumer", createMemberMetadata({"topic"})}
        };
        
        std::map<std::string, std::vector<int32_t>> topics = {
            {"topic", {0, 1, 2}}
        };
        
        BalanceStrategyPlan plan;
        EXPECT_EQ(strategy->Plan(members, topics, plan), 0) << "Sticky plan should succeed for scenario 2";
        EXPECT_FALSE(plan.empty()) << "Sticky plan should not be empty for scenario 2";
    }
    
    // Scenario 3: Two consumers with two topics and six partitions
    {
        std::map<std::string, ConsumerGroupMemberMetadata> members = {
            {"consumer1", createMemberMetadata({"topic1", "topic2"})},
            {"consumer2", createMemberMetadata({"topic1", "topic2"})}
        };
        
        std::map<std::string, std::vector<int32_t>> topics = {
            {"topic1", {0, 1, 2}},
            {"topic2", {0, 1, 2}}
        };
        
        BalanceStrategyPlan plan;
        EXPECT_EQ(strategy->Plan(members, topics, plan), 0) << "Sticky plan should succeed for scenario 3";
        EXPECT_FALSE(plan.empty()) << "Sticky plan should not be empty for scenario 3";
    }
}

// Test RoundRobin Balance Strategy
TEST_F(BalanceStrategyTest, RoundRobinStrategyName) {
    auto strategy = NewBalanceStrategyRoundRobin();
    EXPECT_EQ(strategy->Name(), "roundrobin") << "RoundRobin strategy should return correct name";
}

TEST_F(BalanceStrategyTest, RoundRobinStrategyPlan) {
    auto strategy = NewBalanceStrategyRoundRobin();
    BalanceStrategyPlan plan;
    
    int result = strategy->Plan(testMembers, testTopics, plan);
    EXPECT_EQ(result, 0) << "RoundRobin strategy Plan should succeed";
    EXPECT_FALSE(plan.empty()) << "RoundRobin strategy should generate non-empty plan";
}

TEST_F(BalanceStrategyTest, RoundRobinStrategyAssignmentData) {
    auto strategy = NewBalanceStrategyRoundRobin();
    std::string data;
    
    std::map<std::string, std::vector<int32_t>> memberTopics = {
        {"topic1", {0, 1, 2, 3}},
        {"topic2", {0, 1, 2}},
        {"topic3", {0, 1}}
    };
    
    int result = strategy->AssignmentData("test-member", memberTopics, 3, data);
    EXPECT_EQ(result, 0) << "RoundRobin strategy AssignmentData should succeed";
}

// Test RoundRobin Balance Strategy with specific scenarios from Sarama
TEST_F(BalanceStrategyTest, RoundRobinStrategyPlanScenarios) {
    auto strategy = NewBalanceStrategyRoundRobin();
    
    // Scenario 1: 2 members, 3 topics, different partitions
    {
        std::map<std::string, ConsumerGroupMemberMetadata> members = {
            {"M1", createMemberMetadata({"T1", "T2", "T3"})},
            {"M2", createMemberMetadata({"T1", "T2", "T3"})}
        };
        
        std::map<std::string, std::vector<int32_t>> topics = {
            {"T1", {0}},
            {"T2", {0, 1}},
            {"T3", {0, 1, 2, 3}}
        };
        
        BalanceStrategyPlan plan;
        EXPECT_EQ(strategy->Plan(members, topics, plan), 0) << "RoundRobin plan should succeed for scenario 1";
        EXPECT_FALSE(plan.empty()) << "RoundRobin plan should not be empty for scenario 1";
    }
    
    // Scenario 2: 2 members, 1 topic
    {
        std::map<std::string, ConsumerGroupMemberMetadata> members = {
            {"M1", createMemberMetadata({"T1"})},
            {"M2", createMemberMetadata({"T1"})}
        };
        
        std::map<std::string, std::vector<int32_t>> topics = {
            {"T1", {0}}
        };
        
        BalanceStrategyPlan plan;
        EXPECT_EQ(strategy->Plan(members, topics, plan), 0) << "RoundRobin plan should succeed for scenario 2";
        EXPECT_FALSE(plan.empty()) << "RoundRobin plan should not be empty for scenario 2";
    }
}

// Test factory functions
TEST_F(BalanceStrategyTest, FactoryFunctions) {
    // Test Range factory
    auto rangeStrategy = NewBalanceStrategyRange();
    EXPECT_NE(rangeStrategy, nullptr) << "Range factory should return valid strategy";
    EXPECT_EQ(rangeStrategy->Name(), "range") << "Range factory should return correct strategy type";
    
    // Test Sticky factory
    auto stickyStrategy = NewBalanceStrategySticky();
    EXPECT_NE(stickyStrategy, nullptr) << "Sticky factory should return valid strategy";
    EXPECT_EQ(stickyStrategy->Name(), "sticky") << "Sticky factory should return correct strategy type";
    
    // Test RoundRobin factory
    auto roundRobinStrategy = NewBalanceStrategyRoundRobin();
    EXPECT_NE(roundRobinStrategy, nullptr) << "RoundRobin factory should return valid strategy";
    EXPECT_EQ(roundRobinStrategy->Name(), "roundrobin") << "RoundRobin factory should return correct strategy type";
}

// Test with single member
TEST_F(BalanceStrategyTest, SingleMemberPlan) {
    std::map<std::string, ConsumerGroupMemberMetadata> singleMember;
    singleMember["only-member"].m_version = 0;
    singleMember["only-member"].m_topics = {"topic1", "topic2", "topic3"};
    
    // Test all strategies with single member
    auto rangeStrategy = NewBalanceStrategyRange();
    auto stickyStrategy = NewBalanceStrategySticky();
    auto roundRobinStrategy = NewBalanceStrategyRoundRobin();
    
    BalanceStrategyPlan rangePlan, stickyPlan, roundRobinPlan;
    
    EXPECT_EQ(rangeStrategy->Plan(singleMember, testTopics, rangePlan), 0) << "Range should handle single member";
    EXPECT_EQ(stickyStrategy->Plan(singleMember, testTopics, stickyPlan), 0) << "Sticky should handle single member";
    EXPECT_EQ(roundRobinStrategy->Plan(singleMember, testTopics, roundRobinPlan), 0) << "RoundRobin should handle single member";
}

// Simple test that doesn't use the test fixture, to isolate issues
TEST(BalanceStrategySimpleTest, NoTopicsPlanSimple) {
    std::map<std::string, std::vector<int32_t>> emptyTopics;
    
    // Create members with no topic subscriptions
    std::map<std::string, ConsumerGroupMemberMetadata> members;
    members["member1"].m_version = 0;
    members["member1"].m_topics = {};
    
    // Test only Range strategy first
    auto rangeStrategy = NewBalanceStrategyRange();
    BalanceStrategyPlan rangePlan;
    
    int result = rangeStrategy->Plan(members, emptyTopics, rangePlan);
    EXPECT_EQ(result, 0) << "Range should return 0 for no topics";
    EXPECT_TRUE(rangePlan.empty()) << "Range plan should be empty with no topics";
}

// Test with single topic
TEST_F(BalanceStrategyTest, SingleTopicPlan) {
    std::map<std::string, std::vector<int32_t>> singleTopic;
    singleTopic["topic1"] = {0, 1, 2, 3, 4};
    
    // Create members subscribed only to topic1 for this test
    std::map<std::string, ConsumerGroupMemberMetadata> singleTopicMembers;
    singleTopicMembers["member1"].m_version = 0;
    singleTopicMembers["member1"].m_topics = {"topic1"};
    
    singleTopicMembers["member2"].m_version = 0;
    singleTopicMembers["member2"].m_topics = {"topic1"};
    
    // Test all strategies with single topic
    auto rangeStrategy = NewBalanceStrategyRange();
    auto stickyStrategy = NewBalanceStrategySticky();
    auto roundRobinStrategy = NewBalanceStrategyRoundRobin();
    
    BalanceStrategyPlan rangePlan, stickyPlan, roundRobinPlan;
    
    EXPECT_EQ(rangeStrategy->Plan(singleTopicMembers, singleTopic, rangePlan), 0) << "Range should handle single topic";
    EXPECT_EQ(stickyStrategy->Plan(singleTopicMembers, singleTopic, stickyPlan), 0) << "Sticky should handle single topic";
    EXPECT_EQ(roundRobinStrategy->Plan(singleTopicMembers, singleTopic, roundRobinPlan), 0) << "RoundRobin should handle single topic";
    
    EXPECT_FALSE(rangePlan.empty()) << "Range plan should not be empty with single topic";
    EXPECT_FALSE(stickyPlan.empty()) << "Sticky plan should not be empty with single topic";
    EXPECT_FALSE(roundRobinPlan.empty()) << "RoundRobin plan should not be empty with single topic";
}

// Test with mixed topic subscriptions
TEST_F(BalanceStrategyTest, MixedTopicSubscriptions) {
    std::map<std::string, ConsumerGroupMemberMetadata> members = {
        {"consumer1", createMemberMetadata({"topic1", "topic2"})},
        {"consumer2", createMemberMetadata({"topic2", "topic3"})},
        {"consumer3", createMemberMetadata({"topic1", "topic3"})}
    };
    
    std::map<std::string, std::vector<int32_t>> topics = {
        {"topic1", {0, 1}},
        {"topic2", {0, 1, 2}},
        {"topic3", {0}}
    };
    
    // Test all strategies with mixed subscriptions
    auto rangeStrategy = NewBalanceStrategyRange();
    auto stickyStrategy = NewBalanceStrategySticky();
    auto roundRobinStrategy = NewBalanceStrategyRoundRobin();
    
    BalanceStrategyPlan rangePlan, stickyPlan, roundRobinPlan;
    
    EXPECT_EQ(rangeStrategy->Plan(members, topics, rangePlan), 0) << "Range should handle mixed subscriptions";
    EXPECT_EQ(stickyStrategy->Plan(members, topics, stickyPlan), 0) << "Sticky should handle mixed subscriptions";
    EXPECT_EQ(roundRobinStrategy->Plan(members, topics, roundRobinPlan), 0) << "RoundRobin should handle mixed subscriptions";
    
    EXPECT_FALSE(rangePlan.empty()) << "Range plan should not be empty with mixed subscriptions";
    EXPECT_FALSE(stickyPlan.empty()) << "Sticky plan should not be empty with mixed subscriptions";
    EXPECT_FALSE(roundRobinPlan.empty()) << "RoundRobin plan should not be empty with mixed subscriptions";
}