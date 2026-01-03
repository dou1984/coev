#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <coev/coev.h>
#include "consumer_group_members.h"
#include "sticky_assignor_user_data.h"

struct consumerGenerationPair
{
    std::string MemberID;
    int Generation;
};

using BalanceStrategyPlan = std::map<std::string, std::map<std::string, std::vector<int32_t>>>;

struct BalanceStrategy
{
    virtual ~BalanceStrategy() = default;
    virtual std::string Name() = 0;
    virtual int Plan(const std::map<std::string, ConsumerGroupMemberMetadata> &members,
                     const std::map<std::string, std::vector<int32_t>> &topics,
                     BalanceStrategyPlan &plan) = 0;
    virtual int AssignmentData(const std::string &memberID,
                               const std::map<std::string, std::vector<int32_t>> &topics,
                               int32_t generationID,
                               std::string &data) = 0;
};

std::shared_ptr<BalanceStrategy> NewBalanceStrategyRange();

std::shared_ptr<BalanceStrategy> NewBalanceStrategySticky();

std::shared_ptr<BalanceStrategy> NewBalanceStrategyRoundRobin();
