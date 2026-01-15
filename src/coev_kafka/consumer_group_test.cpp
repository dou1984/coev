#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <string>
#include <map>
#include "consumer_group.h"
#include "config.h"
#include "balance_strategy.h"

class ConsumerGroupTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试配置
        config = std::make_shared<Config>();
        config->Version = V2_5_0_0; // 设置较新的Kafka版本以支持所有功能
    }
    
    void TearDown() override {
        // 清理资源
        config.reset();
    }
    
    std::shared_ptr<Config> config;
};

// 测试平衡策略工厂函数
TEST_F(ConsumerGroupTest, BalanceStrategyFactories) {
    // 测试各种平衡策略的工厂函数
    auto rangeStrategy = NewBalanceStrategyRange();
    auto roundRobinStrategy = NewBalanceStrategyRoundRobin();
    auto stickyStrategy = NewBalanceStrategySticky();
    
    // 验证策略创建成功
    EXPECT_NE(rangeStrategy, nullptr);
    EXPECT_NE(roundRobinStrategy, nullptr);
    EXPECT_NE(stickyStrategy, nullptr);
    
    // 验证策略名称
    EXPECT_EQ(rangeStrategy->Name(), "range");
    EXPECT_EQ(roundRobinStrategy->Name(), "roundrobin");
    EXPECT_EQ(stickyStrategy->Name(), "sticky");
}

// 测试Range平衡策略计划生成
TEST_F(ConsumerGroupTest, RangeBalanceStrategyPlan) {
    // 创建Range平衡策略
    auto rangeStrategy = NewBalanceStrategyRange();
    
    // 创建测试数据
    std::map<std::string, ConsumerGroupMemberMetadata> members;
    std::map<std::string, std::vector<int32_t>> topics;
    BalanceStrategyPlan plan;
    
    // 添加测试成员
    ConsumerGroupMemberMetadata member1;
    member1.m_topics = {"test-topic"};
    members["member1"] = member1;
    
    ConsumerGroupMemberMetadata member2;
    member2.m_topics = {"test-topic"};
    members["member2"] = member2;
    
    // 添加测试主题分区
    topics["test-topic"] = {0, 1, 2, 3};
    
    // 测试Range平衡策略
    int err = rangeStrategy->Plan(members, topics, plan);
    EXPECT_EQ(err, 0);
    EXPECT_FALSE(plan.empty());
    
    // 验证计划包含预期的成员
    EXPECT_TRUE(plan.find("member1") != plan.end());
    EXPECT_TRUE(plan.find("member2") != plan.end());
    
    // 验证Range策略的分区分配（member1应该获得更多分区）
    EXPECT_GE(plan["member1"]["test-topic"].size(), 2);
    EXPECT_GE(plan["member2"]["test-topic"].size(), 2);
}

// 测试RoundRobin平衡策略计划生成
TEST_F(ConsumerGroupTest, RoundRobinBalanceStrategyPlan) {
    // 创建RoundRobin平衡策略
    auto roundRobinStrategy = NewBalanceStrategyRoundRobin();
    
    // 创建测试数据
    std::map<std::string, ConsumerGroupMemberMetadata> members;
    std::map<std::string, std::vector<int32_t>> topics;
    BalanceStrategyPlan plan;
    
    // 添加测试成员
    ConsumerGroupMemberMetadata member1;
    member1.m_topics = {"test-topic"};
    members["member1"] = member1;
    
    ConsumerGroupMemberMetadata member2;
    member2.m_topics = {"test-topic"};
    members["member2"] = member2;
    
    // 添加测试主题分区
    topics["test-topic"] = {0, 1, 2, 3};
    
    // 测试RoundRobin平衡策略
    int err = roundRobinStrategy->Plan(members, topics, plan);
    EXPECT_EQ(err, 0);
    EXPECT_FALSE(plan.empty());
    
    // 验证计划包含预期的成员
    EXPECT_TRUE(plan.find("member1") != plan.end());
    EXPECT_TRUE(plan.find("member2") != plan.end());
    
    // 验证RoundRobin策略的分区分配（应该大致平均）
    EXPECT_EQ(plan["member1"]["test-topic"].size(), 2);
    EXPECT_EQ(plan["member2"]["test-topic"].size(), 2);
}

// 测试Sticky平衡策略计划生成
TEST_F(ConsumerGroupTest, StickyBalanceStrategyPlan) {
    // 创建Sticky平衡策略
    auto stickyStrategy = NewBalanceStrategySticky();
    
    // 创建测试数据
    std::map<std::string, ConsumerGroupMemberMetadata> members;
    std::map<std::string, std::vector<int32_t>> topics;
    BalanceStrategyPlan plan;
    
    // 添加测试成员
    ConsumerGroupMemberMetadata member1;
    member1.m_topics = {"test-topic"};
    members["member1"] = member1;
    
    ConsumerGroupMemberMetadata member2;
    member2.m_topics = {"test-topic"};
    members["member2"] = member2;
    
    // 添加测试主题分区
    topics["test-topic"] = {0, 1, 2, 3};
    
    // 测试Sticky平衡策略
    int err = stickyStrategy->Plan(members, topics, plan);
    EXPECT_EQ(err, 0);
    EXPECT_FALSE(plan.empty());
    
    // 验证计划包含预期的成员
    EXPECT_TRUE(plan.find("member1") != plan.end());
    EXPECT_TRUE(plan.find("member2") != plan.end());
}

// 测试AssignmentData方法
TEST_F(ConsumerGroupTest, AssignmentData) {
    // 测试各种策略的AssignmentData方法
    auto rangeStrategy = NewBalanceStrategyRange();
    auto roundRobinStrategy = NewBalanceStrategyRoundRobin();
    auto stickyStrategy = NewBalanceStrategySticky();
    
    std::string memberID = "test-member";
    std::map<std::string, std::vector<int32_t>> topics;
    topics["test-topic"] = {0, 1};
    int32_t generationID = 1;
    
    // 测试Range策略
    std::string rangeData;
    int err = rangeStrategy->AssignmentData(memberID, topics, generationID, rangeData);
    EXPECT_EQ(err, 0);
    
    // 测试RoundRobin策略
    std::string roundRobinData;
    err = roundRobinStrategy->AssignmentData(memberID, topics, generationID, roundRobinData);
    EXPECT_EQ(err, 0);
    
    // 测试Sticky策略
    std::string stickyData;
    err = stickyStrategy->AssignmentData(memberID, topics, generationID, stickyData);
    EXPECT_EQ(err, 0);
}

// 测试BalanceStrategyPlan结构
TEST_F(ConsumerGroupTest, BalanceStrategyPlanStructure) {
    // 测试BalanceStrategyPlan结构的基本操作
    BalanceStrategyPlan plan;
    
    // 添加一些测试数据
    plan["member1"]["test-topic1"] = {0, 1};
    plan["member1"]["test-topic2"] = {0};
    plan["member2"]["test-topic1"] = {2, 3};
    
    // 验证数据结构
    EXPECT_EQ(plan.size(), 2);
    EXPECT_EQ(plan["member1"].size(), 2);
    EXPECT_EQ(plan["member2"].size(), 1);
    EXPECT_EQ(plan["member1"]["test-topic1"].size(), 2);
    EXPECT_EQ(plan["member1"]["test-topic2"].size(), 1);
    EXPECT_EQ(plan["member2"]["test-topic1"].size(), 2);
    
    // 验证具体值
    EXPECT_EQ(plan["member1"]["test-topic1"][0], 0);
    EXPECT_EQ(plan["member1"]["test-topic1"][1], 1);
    EXPECT_EQ(plan["member1"]["test-topic2"][0], 0);
    EXPECT_EQ(plan["member2"]["test-topic1"][0], 2);
    EXPECT_EQ(plan["member2"]["test-topic1"][1], 3);
}

// 测试ConsumerGroupMemberMetadata结构
TEST_F(ConsumerGroupTest, ConsumerGroupMemberMetadata) {
    // 测试ConsumerGroupMemberMetadata结构的基本操作
    ConsumerGroupMemberMetadata metadata;
    
    // 设置数据
    metadata.m_topics = {"topic1", "topic2", "topic3"};
    metadata.m_user_data = "test-user-data";
    
    // 验证数据
    EXPECT_EQ(metadata.m_topics.size(), 3);
    EXPECT_EQ(metadata.m_topics[0], "topic1");
    EXPECT_EQ(metadata.m_topics[1], "topic2");
    EXPECT_EQ(metadata.m_topics[2], "topic3");
    EXPECT_EQ(metadata.m_user_data, "test-user-data");
    
    // 测试修改
    metadata.m_topics.push_back("topic4");
    metadata.m_user_data = "updated-user-data";
    
    // 验证修改
    EXPECT_EQ(metadata.m_topics.size(), 4);
    EXPECT_EQ(metadata.m_topics[3], "topic4");
    EXPECT_EQ(metadata.m_user_data, "updated-user-data");
}

// 测试consumerGenerationPair结构
TEST_F(ConsumerGroupTest, ConsumerGenerationPair) {
    // 测试consumerGenerationPair结构
    consumerGenerationPair pair;
    
    // 设置数据
    pair.m_member_id = "test-member-123";
    pair.m_generation = 5;
    
    // 验证数据
    EXPECT_EQ(pair.m_member_id, "test-member-123");
    EXPECT_EQ(pair.m_generation, 5);
    
    // 测试修改
    pair.m_member_id = "updated-member";
    pair.m_generation = 6;
    
    // 验证修改
    EXPECT_EQ(pair.m_member_id, "updated-member");
    EXPECT_EQ(pair.m_generation, 6);
}

// 测试多主题的平衡策略
TEST_F(ConsumerGroupTest, MultiTopicBalanceStrategy) {
    // 创建RoundRobin平衡策略
    auto roundRobinStrategy = NewBalanceStrategyRoundRobin();
    
    // 创建测试数据 - 多个主题
    std::map<std::string, ConsumerGroupMemberMetadata> members;
    std::map<std::string, std::vector<int32_t>> topics;
    BalanceStrategyPlan plan;
    
    // 添加测试成员
    ConsumerGroupMemberMetadata member1;
    member1.m_topics = {"topic1", "topic2"};
    members["member1"] = member1;
    
    ConsumerGroupMemberMetadata member2;
    member2.m_topics = {"topic1", "topic2"};
    members["member2"] = member2;
    
    // 添加测试主题分区
    topics["topic1"] = {0, 1};
    topics["topic2"] = {0, 1, 2};
    
    // 测试RoundRobin平衡策略
    int err = roundRobinStrategy->Plan(members, topics, plan);
    EXPECT_EQ(err, 0);
    EXPECT_FALSE(plan.empty());
    
    // 验证计划包含所有主题
    EXPECT_FALSE(plan["member1"]["topic1"].empty());
    EXPECT_FALSE(plan["member1"]["topic2"].empty());
    EXPECT_FALSE(plan["member2"]["topic1"].empty());
    EXPECT_FALSE(plan["member2"]["topic2"].empty());
}

// 测试单个成员的平衡策略
TEST_F(ConsumerGroupTest, SingleMemberBalanceStrategy) {
    // 创建Range平衡策略
    auto rangeStrategy = NewBalanceStrategyRange();
    
    // 创建测试数据 - 单个成员
    std::map<std::string, ConsumerGroupMemberMetadata> members;
    std::map<std::string, std::vector<int32_t>> topics;
    BalanceStrategyPlan plan;
    
    // 添加测试成员
    ConsumerGroupMemberMetadata member1;
    member1.m_topics = {"test-topic"};
    members["member1"] = member1;
    
    // 添加测试主题分区
    topics["test-topic"] = {0, 1, 2, 3};
    
    // 测试Range平衡策略
    int err = rangeStrategy->Plan(members, topics, plan);
    EXPECT_EQ(err, 0);
    EXPECT_FALSE(plan.empty());
    
    // 验证单个成员获得所有分区
    EXPECT_EQ(plan["member1"]["test-topic"].size(), 4);
}
