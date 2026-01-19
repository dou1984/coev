#include "balance_strategy.h"
#include "topic_partition_assignment.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <set>
#include <queue>
#include <unordered_map>
#include <unordered_set>

const std::string RangeBalanceStrategyName = "range";
const std::string RoundRobinBalanceStrategyName = "roundrobin";
const std::string StickyBalanceStrategyName = "sticky";
const int defaultGeneration = -1;

void AddToPlan(BalanceStrategyPlan &p, const std::string &memberID, const std::string &topic, const std::vector<int32_t> &partitions)
{
    if (partitions.empty())
        return;

    p[memberID][topic].insert(p[memberID][topic].end(), partitions.begin(), partitions.end());
}

struct balanceStrategy : BalanceStrategy
{
    using CoreFn = std::function<void(BalanceStrategyPlan &, std::vector<std::string> &, const std::string &, const std::vector<int32_t> &)>;

    balanceStrategy(CoreFn fn, const std::string &name) : coreFn(fn), name(name) {}

    std::string Name() { return name; }

    int Plan(const std::map<std::string, ConsumerGroupMemberMetadata> &members, const std::map<std::string, std::vector<int32_t>> &topics, BalanceStrategyPlan &plan)
    {
        std::map<std::string, std::vector<std::string>> mbt;
        for (auto &it : members)
        {
            auto &memberId = it.first;
            auto &meta = it.second;
            for (auto &topic : meta.m_topics)
            {
                mbt[topic].push_back(memberId);
            }
        }

        auto uniq = [](std::vector<std::string> &ss) -> std::vector<std::string>
        {
            if (ss.size() < 2)
                return ss;
            std::sort(ss.begin(), ss.end());
            auto last = std::unique(ss.begin(), ss.end());
            ss.erase(last, ss.end());
            return ss;
        };

        plan.clear();

        for (auto &[topic, memberIDs] : mbt)
        {
            auto it = topics.find(topic);
            if (it == topics.end())
            {
                continue; // Skip topics that don't exist in the topics map
            }
            auto uniqueMembers = uniq(memberIDs);
            coreFn(plan, uniqueMembers, topic, it->second);
        }
        return 0;
    }

    int AssignmentData(const std::string &memberID, const std::map<std::string, std::vector<int32_t>> &topics,
                       int32_t generationID, std::string &data)
    {
        data.clear();
        return 0;
    }

private:
    CoreFn coreFn;
    std::string name;
};

std::shared_ptr<BalanceStrategy> NewBalanceStrategyRange()
{
    auto coreFn = [](BalanceStrategyPlan &plan, std::vector<std::string> &memberIDs, const std::string &topic, const std::vector<int32_t> &partitions)
    {
        int partitionsPerConsumer = static_cast<int>(partitions.size()) / static_cast<int>(memberIDs.size());
        int consumersWithExtraPartition = static_cast<int>(partitions.size()) % static_cast<int>(memberIDs.size());
        std::sort(memberIDs.begin(), memberIDs.end());
        for (size_t i = 0; i < memberIDs.size(); ++i)
        {
            int min = static_cast<int>(i) * partitionsPerConsumer + static_cast<int>(std::min(static_cast<double>(consumersWithExtraPartition), static_cast<double>(i)));
            int extra = (i < static_cast<size_t>(consumersWithExtraPartition)) ? 1 : 0;
            int max = min + partitionsPerConsumer + extra;
            std::vector<int32_t> slice(partitions.begin() + min, partitions.begin() + max);
            AddToPlan(plan, memberIDs[i], topic, slice);
        }
    };
    return std::make_shared<balanceStrategy>(coreFn, RangeBalanceStrategyName);
}

std::shared_ptr<BalanceStrategy> BalanceStrategyRange = NewBalanceStrategyRange();

struct consumerPair
{
    std::string SrcMemberID;
    std::string DstMemberID;
    bool operator==(const consumerPair &other) const
    {
        return SrcMemberID == other.SrcMemberID && DstMemberID == other.DstMemberID;
    }
    bool operator<(const consumerPair &other) const
    {
        return SrcMemberID < other.SrcMemberID || (SrcMemberID == other.SrcMemberID && DstMemberID < other.DstMemberID);
    }
};

struct partitionMovements
{
    std::map<std::string, std::map<consumerPair, std::map<TopicPartitionAssignment, bool>>> PartitionMovementsByTopic;
    std::map<TopicPartitionAssignment, consumerPair> Movements;

    void removeMovementRecordOfPartition(const TopicPartitionAssignment &partition, consumerPair &outPair);
    void addPartitionMovementRecord(const TopicPartitionAssignment &partition, const consumerPair &pair);
    void movePartition(const TopicPartitionAssignment &partition, const std::string &oldConsumer, const std::string &newConsumer);
    TopicPartitionAssignment getTheActualPartitionToBeMoved(const TopicPartitionAssignment &partition, const std::string &oldConsumer, const std::string &newConsumer);
    std::pair<std::vector<std::string>, bool> isLinked(const std::string &src, const std::string &dst, std::vector<consumerPair> pairs, std::vector<std::string> currentPath);
    bool in(const std::vector<std::string> &cycle, const std::vector<std::vector<std::string>> &cycles);
    bool hasCycles(const std::vector<consumerPair> &pairs);
    bool isSticky();
};

struct stickyBalanceStrategy : BalanceStrategy
{
    std::string Name()
    {
        return StickyBalanceStrategyName;
    }

    int Plan(const std::map<std::string, ConsumerGroupMemberMetadata> &members,
             const std::map<std::string, std::vector<int32_t>> &topics,
             BalanceStrategyPlan &plan);

    int AssignmentData(const std::string &memberID,
                       const std::map<std::string, std::vector<int32_t>> &topics,
                       int32_t generationID,
                       std::string &data);

private:
    partitionMovements movements;

    void balance(std::map<std::string, std::vector<TopicPartitionAssignment>> &currentAssignment,
                 const std::map<TopicPartitionAssignment, consumerGenerationPair> &prevAssignment,
                 std::vector<TopicPartitionAssignment> &sortedPartitions,
                 std::vector<TopicPartitionAssignment> &unassignedPartitions,
                 std::vector<std::string> &sortedCurrentSubscriptions,
                 const std::map<std::string, std::vector<TopicPartitionAssignment>> &consumer2AllPotentialPartitions,
                 const std::map<TopicPartitionAssignment, std::vector<std::string>> &partition2AllPotentialConsumers,
                 std::map<TopicPartitionAssignment, std::string> &currentPartitionConsumer);

    bool performReassignments(std::vector<TopicPartitionAssignment> &reassignablePartitions,
                              std::map<std::string, std::vector<TopicPartitionAssignment>> &currentAssignment,
                              const std::map<TopicPartitionAssignment, consumerGenerationPair> &prevAssignment,
                              std::vector<std::string> &sortedCurrentSubscriptions,
                              const std::map<std::string, std::vector<TopicPartitionAssignment>> &consumer2AllPotentialPartitions,
                              const std::map<TopicPartitionAssignment, std::vector<std::string>> &partition2AllPotentialConsumers,
                              std::map<TopicPartitionAssignment, std::string> &currentPartitionConsumer);

    std::vector<std::string> reassignPartitionToNewConsumer(const TopicPartitionAssignment &partition,
                                                            std::map<std::string, std::vector<TopicPartitionAssignment>> &currentAssignment,
                                                            std::vector<std::string> &sortedCurrentSubscriptions,
                                                            std::map<TopicPartitionAssignment, std::string> &currentPartitionConsumer,
                                                            const std::map<std::string, std::vector<TopicPartitionAssignment>> &consumer2AllPotentialPartitions);

    std::vector<std::string> reassignPartition(const TopicPartitionAssignment &partition,
                                               std::map<std::string, std::vector<TopicPartitionAssignment>> &currentAssignment,
                                               std::vector<std::string> &sortedCurrentSubscriptions,
                                               std::map<TopicPartitionAssignment, std::string> &currentPartitionConsumer,
                                               const std::string &newConsumer);

    std::vector<std::string> processPartitionMovement(const TopicPartitionAssignment &partition,
                                                      const std::string &newConsumer,
                                                      std::map<std::string, std::vector<TopicPartitionAssignment>> &currentAssignment,
                                                      std::vector<std::string> &sortedCurrentSubscriptions,
                                                      std::map<TopicPartitionAssignment, std::string> &currentPartitionConsumer);
};

std::shared_ptr<BalanceStrategy> NewBalanceStrategySticky()
{
    return std::make_shared<stickyBalanceStrategy>();
}

std::shared_ptr<BalanceStrategy> BalanceStrategySticky = NewBalanceStrategySticky();

struct topicAndPartition
{
    std::string topic;
    int32_t partition;
    std::string comparedValue() const
    {
        return topic + "-" + std::to_string(partition);
    }
    bool operator<(const topicAndPartition &other) const
    {
        return comparedValue() < other.comparedValue();
    }
};

struct memberAndTopic
{
    std::unordered_set<std::string> topics;
    std::string memberID;
    bool hasTopic(const std::string &topic) const
    {
        return topics.count(topic) > 0;
    }
};

class roundRobinBalancer : public BalanceStrategy
{
public:
    std::string Name() { return RoundRobinBalanceStrategyName; }

    int Plan(const std::map<std::string, ConsumerGroupMemberMetadata> &memberAndMetadata,
             const std::map<std::string, std::vector<int32_t>> &topics,
             BalanceStrategyPlan &plan);

    int AssignmentData(const std::string &memberID,
                       const std::map<std::string, std::vector<int32_t>> &topics,
                       int32_t generationID,
                       std::string &data);
};

std::shared_ptr<BalanceStrategy> NewBalanceStrategyRoundRobin()
{
    return std::make_shared<roundRobinBalancer>();
}

std::shared_ptr<BalanceStrategy> BalanceStrategyRoundRobin = NewBalanceStrategyRoundRobin();

// Implementations below

int getBalanceScore(const std::map<std::string, std::vector<TopicPartitionAssignment>> &assignment)
{
    std::map<std::string, int> consumer2AssignmentSize;
    for (auto &[memberID, partitions] : assignment)
    {
        consumer2AssignmentSize[memberID] = static_cast<int>(partitions.size());
    }
    double score = 0.0;
    for (auto it1 = consumer2AssignmentSize.begin(); it1 != consumer2AssignmentSize.end(); ++it1)
    {
        auto tempMap = consumer2AssignmentSize;
        tempMap.erase(it1->first);
        for (auto &[_, size] : tempMap)
        {
            score += std::abs(static_cast<double>(it1->second - size));
        }
    }
    return static_cast<int>(score);
}

bool isBalanced(const std::map<std::string, std::vector<TopicPartitionAssignment>> &currentAssignment,
                const std::map<std::string, std::vector<TopicPartitionAssignment>> &allSubscriptions)
{
    if (currentAssignment.empty())
    {
        return true; // An empty assignment is trivially balanced
    }

    std::vector<std::string> sortedCurrentSubscriptions;
    for (auto &it : currentAssignment)
    {
        sortedCurrentSubscriptions.push_back(it.first);
    }
    std::sort(sortedCurrentSubscriptions.begin(), sortedCurrentSubscriptions.end(),
              [&](const std::string &a, const std::string &b)
              {
                  int da = static_cast<int>(currentAssignment.at(a).size());
                  int db = static_cast<int>(currentAssignment.at(b).size());
                  if (da == db)
                      return a < b;
                  return da < db;
              });
    int min = static_cast<int>(currentAssignment.at(sortedCurrentSubscriptions.front()).size());
    int max = static_cast<int>(currentAssignment.at(sortedCurrentSubscriptions.back()).size());
    if (min >= max - 1)
        return true;

    std::map<TopicPartitionAssignment, std::string> allPartitions_;
    for (auto &[memberID, partitions] : currentAssignment)
    {
        for (auto &partition : partitions)
        {
            if (allPartitions_.count(partition))
            {
                // Log: duplicate assignment
            }
            allPartitions_[partition] = memberID;
        }
    }

    for (auto &memberID : sortedCurrentSubscriptions)
    {
        auto &consumerPartitions = currentAssignment.at(memberID);
        int consumerPartitionCount = static_cast<int>(consumerPartitions.size());
        if (consumerPartitionCount == static_cast<int>(allSubscriptions.at(memberID).size()))
            continue;
        auto &potentialTopicPartitions = allSubscriptions.at(memberID);
        for (auto &partition : potentialTopicPartitions)
        {
            bool found = false;
            for (auto &p : consumerPartitions)
            {
                if (p == partition)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                auto &otherConsumer = allPartitions_.at(partition);
                int otherConsumerPartitionCount = static_cast<int>(currentAssignment.at(otherConsumer).size());
                if (consumerPartitionCount < otherConsumerPartitionCount)
                {
                    return false;
                }
            }
        }
    }
    return true;
}

bool canConsumerParticipateInReassignment(
    const std::string &memberID,
    const std::map<std::string, std::vector<TopicPartitionAssignment>> &currentAssignment,
    const std::map<std::string, std::vector<TopicPartitionAssignment>> &consumer2AllPotentialPartitions,
    const std::map<TopicPartitionAssignment, std::vector<std::string>> &partition2AllPotentialConsumers)
{
    // Check if memberID exists in currentAssignment
    auto currentIt = currentAssignment.find(memberID);
    if (currentIt == currentAssignment.end())
    {
        // If no current assignment, can participate if they have potential partitions
        auto potentialIt = consumer2AllPotentialPartitions.find(memberID);
        if (potentialIt == consumer2AllPotentialPartitions.end())
        {
            return false;
        }
        return !potentialIt->second.empty();
    }

    auto potentialIt = consumer2AllPotentialPartitions.find(memberID);
    if (potentialIt == consumer2AllPotentialPartitions.end())
    {
        return false;
    }

    const auto &currentPartitions = currentIt->second;
    int currentAssignmentSize = static_cast<int>(currentPartitions.size());
    int maxAssignmentSize = static_cast<int>(potentialIt->second.size());

    if (currentAssignmentSize > maxAssignmentSize)
    {
        // Log: over-assigned
    }
    if (currentAssignmentSize < maxAssignmentSize)
        return true;

    for (auto &partition : currentPartitions)
    {
        auto partitionIt = partition2AllPotentialConsumers.find(partition);
        if (partitionIt != partition2AllPotentialConsumers.end() && partitionIt->second.size() >= 2)
            return true;
    }
    return false;
}

bool canTopicPartitionParticipateInReassignment(
    const TopicPartitionAssignment &partition,
    const std::map<TopicPartitionAssignment, std::vector<std::string>> &partition2AllPotentialConsumers)
{
    auto it = partition2AllPotentialConsumers.find(partition);
    return it != partition2AllPotentialConsumers.end() && it->second.size() >= 2;
}

std::vector<std::string> assignPartition(
    const TopicPartitionAssignment &partition,
    std::vector<std::string> sortedCurrentSubscriptions,
    std::map<std::string, std::vector<TopicPartitionAssignment>> &currentAssignment,
    const std::map<std::string, std::vector<TopicPartitionAssignment>> &consumer2AllPotentialPartitions,
    std::map<TopicPartitionAssignment, std::string> &currentPartitionConsumer)
{
    for (auto &memberID : sortedCurrentSubscriptions)
    {
        bool include = false;
        auto it = consumer2AllPotentialPartitions.find(memberID);
        if (it != consumer2AllPotentialPartitions.end())
        {
            for (auto &p : it->second)
            {
                if (p == partition)
                {
                    include = true;
                    break;
                }
            }
        }
        if (include)
        {
            currentAssignment[memberID].push_back(partition);
            currentPartitionConsumer[partition] = memberID;
            break;
        }
    }
    std::vector<std::string> result;
    for (auto &[id, _] : currentAssignment)
    {
        result.push_back(id);
    }
    std::sort(result.begin(), result.end(),
              [&](const std::string &a, const std::string &b)
              {
                  int da = static_cast<int>(currentAssignment.at(a).size());
                  int db = static_cast<int>(currentAssignment.at(b).size());
                  if (da == db)
                      return a < b;
                  return da < db;
              });
    return result;
}

std::vector<TopicPartitionAssignment> removeTopicPartitionFromMemberAssignments(
    std::vector<TopicPartitionAssignment> assignments,
    const TopicPartitionAssignment &topic)
{
    for (auto it = assignments.begin(); it != assignments.end(); ++it)
    {
        if (*it == topic)
        {
            assignments.erase(it);
            break;
        }
    }
    return assignments;
}

bool memberAssignmentsIncludeTopicPartition(
    const std::vector<TopicPartitionAssignment> &assignments,
    const TopicPartitionAssignment &topic)
{
    for (auto &a : assignments)
    {
        if (a == topic)
            return true;
    }
    return false;
}

std::vector<std::string> sortMemberIDsByPartitionAssignments(
    const std::map<std::string, std::vector<TopicPartitionAssignment>> &assignments)
{
    std::vector<std::string> sortedMemberIDs;
    for (auto &[id, _] : assignments)
    {
        sortedMemberIDs.push_back(id);
    }
    std::sort(sortedMemberIDs.begin(), sortedMemberIDs.end(),
              [&](const std::string &a, const std::string &b)
              {
                  int da = static_cast<int>(assignments.at(a).size());
                  int db = static_cast<int>(assignments.at(b).size());
                  if (da == db)
                      return a < b;
                  return da < db;
              });
    return sortedMemberIDs;
}

std::vector<TopicPartitionAssignment> sortPartitionsByPotentialConsumerAssignments(
    const std::map<TopicPartitionAssignment, std::vector<std::string>> &partition2AllPotentialConsumers)
{
    std::vector<TopicPartitionAssignment> sortedPartitionIDs;
    for (auto &[partition, _] : partition2AllPotentialConsumers)
    {
        sortedPartitionIDs.push_back(partition);
    }
    std::sort(sortedPartitionIDs.begin(), sortedPartitionIDs.end(),
              [&](const TopicPartitionAssignment &a, const TopicPartitionAssignment &b)
              {
                  auto &consumersA = partition2AllPotentialConsumers.at(a);
                  auto &consumersB = partition2AllPotentialConsumers.at(b);
                  if (consumersA.size() == consumersB.size())
                  {
                      if (a.m_topic == b.m_topic)
                      {
                          return a.m_partition < b.m_partition;
                      }
                      return a.m_topic < b.m_topic;
                  }
                  return consumersA.size() < consumersB.size();
              });
    return sortedPartitionIDs;
}

std::map<std::string, std::vector<TopicPartitionAssignment>> deepCopyAssignment(
    const std::map<std::string, std::vector<TopicPartitionAssignment>> &assignment)
{
    std::map<std::string, std::vector<TopicPartitionAssignment>> m;
    for (auto &[memberID, subscriptions] : assignment)
    {
        m[memberID] = subscriptions;
    }
    return m;
}

bool areSubscriptionsIdentical(
    const std::map<TopicPartitionAssignment, std::vector<std::string>> &partition2AllPotentialConsumers,
    const std::map<std::string, std::vector<TopicPartitionAssignment>> &consumer2AllPotentialPartitions)
{
    std::map<std::string, int> curMembers;
    bool first = true;
    for (auto &[_, cur] : partition2AllPotentialConsumers)
    {
        if (first)
        {
            for (auto &elem : cur)
            {
                curMembers[elem]++;
            }
            first = false;
            continue;
        }
        if (curMembers.size() != cur.size())
            return false;
        std::map<std::string, int> yMap;
        for (auto &yElem : cur)
        {
            yMap[yElem]++;
        }
        for (auto &[key, val] : curMembers)
        {
            if (yMap[key] != val)
                return false;
        }
    }

    std::map<TopicPartitionAssignment, int> curPartitions_;
    first = true;
    for (auto &[_, cur] : consumer2AllPotentialPartitions)
    {
        if (first)
        {
            for (auto &elem : cur)
            {
                curPartitions_[elem]++;
            }
            first = false;
            continue;
        }
        if (curPartitions_.size() != cur.size())
            return false;
        std::map<TopicPartitionAssignment, int> yMap_;
        for (auto &yElem : cur)
        {
            yMap_[yElem]++;
        }
        for (auto &[key, val] : curPartitions_)
        {
            if (yMap_[key] != val)
                return false;
        }
    }
    return true;
}

int prepopulateCurrentAssignments(const std::map<std::string, ConsumerGroupMemberMetadata> &members, std::map<std::string, std::vector<TopicPartitionAssignment>> &currentAssignment, std::map<TopicPartitionAssignment, consumerGenerationPair> &prevAssignment)
{

    std::map<TopicPartitionAssignment, std::map<int, std::string>> sortedPartitionConsumersByGeneration;

    for (auto &it : members)
    {
        auto &memberID = it.first;
        auto &meta = it.second;

        std::vector<TopicPartitionAssignment> userPartitions;
        int generation = defaultGeneration;
        bool hasGen = false;

        for (auto &partition : userPartitions)
        {
            auto &consumers = sortedPartitionConsumersByGeneration[partition];
            if (consumers.empty())
            {
                consumers[generation] = memberID;
            }
            else
            {
                if (hasGen)
                {
                    if (consumers.count(generation) == 0)
                    {
                        consumers[generation] = memberID;
                    }
                }
                else
                {
                    consumers[defaultGeneration] = memberID;
                }
            }
        }
    }

    for (auto &it : sortedPartitionConsumersByGeneration)
    {
        auto &partition = it.first;
        auto &consumers = it.second;
        std::vector<int> generations;
        for (auto cit : consumers)
        {
            generations.push_back(cit.first);
        }
        std::sort(generations.rbegin(), generations.rend());
        const std::string &consumer = consumers[generations[0]];
        currentAssignment[consumer].push_back(partition);
        if (generations.size() > 1)
        {
            prevAssignment[partition] = consumerGenerationPair{
                consumers[generations[1]], generations[1]};
        }
    }

    return 0;
}

void partitionMovements::removeMovementRecordOfPartition(const TopicPartitionAssignment &partition, consumerPair &outPair)
{
    outPair = Movements[partition];
    Movements.erase(partition);
    auto &topicMap = PartitionMovementsByTopic[partition.m_topic];
    topicMap[outPair].erase(partition);
    if (topicMap[outPair].empty())
    {
        topicMap.erase(outPair);
    }
    if (PartitionMovementsByTopic[partition.m_topic].empty())
    {
        PartitionMovementsByTopic.erase(partition.m_topic);
    }
}

void partitionMovements::addPartitionMovementRecord(const TopicPartitionAssignment &partition, const consumerPair &pair)
{
    Movements[partition] = pair;
    if (PartitionMovementsByTopic.find(partition.m_topic) == PartitionMovementsByTopic.end())
    {
        PartitionMovementsByTopic[partition.m_topic] = {};
    }
    auto &topicMap = PartitionMovementsByTopic[partition.m_topic];
    if (topicMap.find(pair) == topicMap.end())
    {
        topicMap[pair] = {};
    }
    topicMap[pair][partition] = true;
}

void partitionMovements::movePartition(const TopicPartitionAssignment &partition, const std::string &oldConsumer, const std::string &newConsumer)
{
    consumerPair pair{oldConsumer, newConsumer};
    if (Movements.count(partition))
    {
        consumerPair existingPair;
        removeMovementRecordOfPartition(partition, existingPair);
        if (existingPair.DstMemberID != oldConsumer)
        {
            // Log mismatch
        }
        if (existingPair.SrcMemberID != newConsumer)
        {
            addPartitionMovementRecord(partition, consumerPair{existingPair.SrcMemberID, newConsumer});
        }
    }
    else
    {
        addPartitionMovementRecord(partition, pair);
    }
}

TopicPartitionAssignment partitionMovements::getTheActualPartitionToBeMoved(
    const TopicPartitionAssignment &partition,
    const std::string &oldConsumer,
    const std::string &newConsumer)
{
    if (PartitionMovementsByTopic.count(partition.m_topic) == 0)
    {
        return partition;
    }
    std::string actualOld = oldConsumer;
    if (Movements.count(partition))
    {
        if (Movements[partition].DstMemberID != oldConsumer)
        {
            // Log
        }
        actualOld = Movements[partition].SrcMemberID;
    }
    consumerPair reversePair{newConsumer, actualOld};
    if (PartitionMovementsByTopic[partition.m_topic].count(reversePair) == 0)
    {
        return partition;
    }
    auto &revMap = PartitionMovementsByTopic[partition.m_topic][reversePair];
    return revMap.begin()->first;
}

std::pair<std::vector<std::string>, bool> partitionMovements::isLinked(
    const std::string &src,
    const std::string &dst,
    std::vector<consumerPair> pairs,
    std::vector<std::string> currentPath)
{
    if (src == dst)
    {
        return {currentPath, false};
    }
    if (pairs.empty())
    {
        return {currentPath, false};
    }
    for (auto &pair : pairs)
    {
        if (pair.SrcMemberID == src && pair.DstMemberID == dst)
        {
            currentPath.push_back(src);
            currentPath.push_back(dst);
            return {currentPath, true};
        }
    }
    for (size_t i = 0; i < pairs.size(); ++i)
    {
        if (pairs[i].SrcMemberID != src)
            continue;
        std::vector<consumerPair> reducedSet;
        for (size_t j = 0; j < pairs.size(); ++j)
        {
            if (j != i)
                reducedSet.push_back(pairs[j]);
        }
        currentPath.push_back(src);
        return isLinked(pairs[i].DstMemberID, dst, reducedSet, currentPath);
    }
    return {currentPath, false};
}

bool partitionMovements::in(const std::vector<std::string> &cycle, const std::vector<std::vector<std::string>> &cycles)
{
    if (cycle.size() < 2)
        return false;
    std::vector<std::string> superCycle(cycle.begin(), cycle.end() - 1);
    superCycle.insert(superCycle.end(), cycle.begin(), cycle.end());
    for (auto &foundCycle : cycles)
    {
        if (foundCycle.size() != cycle.size())
            continue;
        bool match = false;
        for (size_t i = 0; i <= superCycle.size() - foundCycle.size(); ++i)
        {
            if (std::equal(foundCycle.begin(), foundCycle.end(), superCycle.begin() + i))
            {
                match = true;
                break;
            }
        }
        if (match)
            return true;
    }
    return false;
}

bool partitionMovements::hasCycles(const std::vector<consumerPair> &pairs)
{
    std::vector<std::vector<std::string>> cycles;
    for (size_t i = 0; i < pairs.size(); ++i)
    {
        std::vector<consumerPair> reducedPairs;
        for (size_t j = 0; j < pairs.size(); ++j)
        {
            if (j != i)
                reducedPairs.push_back(pairs[j]);
        }
        auto [path, linked] = isLinked(pairs[i].DstMemberID, pairs[i].SrcMemberID, reducedPairs, {pairs[i].SrcMemberID});
        if (linked && !in(path, cycles))
        {
            cycles.push_back(path);
            if (path.size() == 3)
                return true;
        }
    }
    return false;
}

bool partitionMovements::isSticky()
{
    for (auto &[topic, movements] : PartitionMovementsByTopic)
    {
        std::vector<consumerPair> movementPairs;
        for (auto &[pair, _] : movements)
        {
            movementPairs.push_back(pair);
        }
        if (hasCycles(movementPairs))
        {
            return false;
        }
    }
    return true;
}

// stickyBalanceStrategy implementations

int stickyBalanceStrategy::Plan(const std::map<std::string, ConsumerGroupMemberMetadata> &members, const std::map<std::string, std::vector<int32_t>> &topics, BalanceStrategyPlan &plan)
{
    // Handle edge case: no topics to assign
    if (topics.empty())
    {
        plan.clear();
        return 0;
    }

    movements = partitionMovements{};
    std::map<std::string, std::vector<TopicPartitionAssignment>> currentAssignment;
    std::map<TopicPartitionAssignment, consumerGenerationPair> prevAssignment;
    auto err = prepopulateCurrentAssignments(members, currentAssignment, prevAssignment);
    bool isFreshAssignment = currentAssignment.empty();

    std::map<TopicPartitionAssignment, std::vector<std::string>> partition2AllPotentialConsumers;
    for (auto &[topic, partitions] : topics)
    {
        for (int32_t p : partitions)
        {
            TopicPartitionAssignment tpa{topic, p};
            partition2AllPotentialConsumers[tpa] = {};
        }
    }

    std::map<std::string, std::vector<TopicPartitionAssignment>> consumer2AllPotentialPartitions;
    for (auto &it : members)
    {
        auto &memberID = it.first;
        auto &meta = it.second;
        std::vector<TopicPartitionAssignment> list;
        for (auto &topicSubscription : meta.m_topics)
        {
            auto topicIt = topics.find(topicSubscription);
            if (topicIt != topics.end())
            {
                for (int32_t p : topicIt->second)
                {
                    TopicPartitionAssignment tpa(topicSubscription, p);
                    list.push_back(tpa);
                    partition2AllPotentialConsumers[tpa].push_back(memberID);
                }
            }
        }
        consumer2AllPotentialPartitions[memberID] = list;
        if (currentAssignment.count(memberID) == 0)
        {
            currentAssignment[memberID] = {};
        }
    }

    std::map<TopicPartitionAssignment, std::string> currentPartitionConsumers;
    std::map<TopicPartitionAssignment, bool> unvisitedPartitions;
    for (auto &[partition, _] : partition2AllPotentialConsumers)
    {
        unvisitedPartitions[partition] = true;
    }

    std::vector<TopicPartitionAssignment> unassignedPartitions;
    for (auto &it : currentAssignment)
    {
        auto &memberID = it.first;
        auto &partitions = it.second;
        std::vector<TopicPartitionAssignment> keepPartitions;
        for (auto &partition : partitions)
        {
            if (partition2AllPotentialConsumers.count(partition) == 0)
                continue;
            unvisitedPartitions.erase(partition);
            currentPartitionConsumers[partition] = memberID;
            bool hasTopic = false;
            for (auto &t : members.at(memberID).m_topics)
            {
                if (t == partition.m_topic)
                {
                    hasTopic = true;
                    break;
                }
            }
            if (!hasTopic)
            {
                unassignedPartitions.push_back(partition);
                continue;
            }
            keepPartitions.push_back(partition);
        }
        currentAssignment[memberID] = keepPartitions;
    }

    for (auto &[unvisited, _] : unvisitedPartitions)
    {
        unassignedPartitions.push_back(unvisited);
    }

    auto sortPartitionsHelper = [&](bool isFresh, auto &currentAss, auto &prevAss,
                                    auto &part2Cons, auto &cons2Parts)
    {
        // Simplified: just return sorted by potential consumers
        return sortPartitionsByPotentialConsumerAssignments(part2Cons);
    };

    std::vector<TopicPartitionAssignment> sortedPartitions = sortPartitionsHelper(
        isFreshAssignment, currentAssignment, prevAssignment,
        partition2AllPotentialConsumers, consumer2AllPotentialPartitions);

    std::vector<std::string> sortedCurrentSubscriptions = sortMemberIDsByPartitionAssignments(currentAssignment);

    balance(currentAssignment, prevAssignment, sortedPartitions, unassignedPartitions,
            sortedCurrentSubscriptions, consumer2AllPotentialPartitions,
            partition2AllPotentialConsumers, currentPartitionConsumers);

    plan.clear();
    for (auto &[memberID, assignments] : currentAssignment)
    {
        if (assignments.empty())
        {
            plan[memberID] = {};
        }
        else
        {
            for (auto &assignment : assignments)
            {
                AddToPlan(plan, memberID, assignment.m_topic, {assignment.m_partition});
            }
        }
    }
    return 0;
}

int stickyBalanceStrategy::AssignmentData(
    const std::string &memberID,
    const std::map<std::string, std::vector<int32_t>> &topics,
    int32_t generationID,
    std::string &data)
{
    // Assume encode exists
    StickyAssignorUserDataV1 userData;
    userData.m_topics = topics;
    userData.m_generation = generationID;
    // encode(&userData, data);
    data.clear(); // placeholder
    return 0;
}

void stickyBalanceStrategy::balance(
    std::map<std::string, std::vector<TopicPartitionAssignment>> &currentAssignment,
    const std::map<TopicPartitionAssignment, consumerGenerationPair> &prevAssignment,
    std::vector<TopicPartitionAssignment> &sortedPartitions,
    std::vector<TopicPartitionAssignment> &unassignedPartitions,
    std::vector<std::string> &sortedCurrentSubscriptions,
    const std::map<std::string, std::vector<TopicPartitionAssignment>> &consumer2AllPotentialPartitions,
    const std::map<TopicPartitionAssignment, std::vector<std::string>> &partition2AllPotentialConsumers,
    std::map<TopicPartitionAssignment, std::string> &currentPartitionConsumer)
{
    bool initializing = sortedCurrentSubscriptions.empty();
    if (!initializing)
    {
        auto it = currentAssignment.find(sortedCurrentSubscriptions[0]);
        initializing = it != currentAssignment.end() && it->second.empty();
    }

    for (auto &partition : unassignedPartitions)
    {
        auto partitionIt = partition2AllPotentialConsumers.find(partition);
        if (partitionIt == partition2AllPotentialConsumers.end() || partitionIt->second.empty())
            continue;
        sortedCurrentSubscriptions = assignPartition(partition, sortedCurrentSubscriptions,
                                                     currentAssignment, consumer2AllPotentialPartitions,
                                                     currentPartitionConsumer);
    }

    for (auto it = partition2AllPotentialConsumers.begin(); it != partition2AllPotentialConsumers.end();)
    {
        if (!canTopicPartitionParticipateInReassignment(it->first, partition2AllPotentialConsumers))
        {
            sortedPartitions.erase(std::remove(sortedPartitions.begin(), sortedPartitions.end(), it->first),
                                   sortedPartitions.end());
            ++it;
        }
        else
        {
            ++it;
        }
    }

    std::map<std::string, std::vector<TopicPartitionAssignment>> fixedAssignments;
    for (auto &[memberID, _] : consumer2AllPotentialPartitions)
    {
        if (!canConsumerParticipateInReassignment(memberID, currentAssignment,
                                                  consumer2AllPotentialPartitions,
                                                  partition2AllPotentialConsumers))
        {
            fixedAssignments[memberID] = currentAssignment[memberID];
            currentAssignment.erase(memberID);
            sortedCurrentSubscriptions = sortMemberIDsByPartitionAssignments(currentAssignment);
        }
    }

    auto preBalanceAssignment = deepCopyAssignment(currentAssignment);
    auto preBalancePartitionConsumers = currentPartitionConsumer;

    bool reassignmentPerformed = performReassignments(sortedPartitions, currentAssignment, prevAssignment,
                                                      sortedCurrentSubscriptions,
                                                      consumer2AllPotentialPartitions,
                                                      partition2AllPotentialConsumers,
                                                      currentPartitionConsumer);

    if (!initializing && reassignmentPerformed &&
        getBalanceScore(currentAssignment) >= getBalanceScore(preBalanceAssignment))
    {
        currentAssignment = deepCopyAssignment(preBalanceAssignment);
        currentPartitionConsumer = preBalancePartitionConsumers;
    }

    for (auto &[id, ass] : fixedAssignments)
    {
        currentAssignment[id] = ass;
    }
}

bool stickyBalanceStrategy::performReassignments(
    std::vector<TopicPartitionAssignment> &reassignablePartitions,
    std::map<std::string, std::vector<TopicPartitionAssignment>> &currentAssignment,
    const std::map<TopicPartitionAssignment, consumerGenerationPair> &prevAssignment,
    std::vector<std::string> &sortedCurrentSubscriptions,
    const std::map<std::string, std::vector<TopicPartitionAssignment>> &consumer2AllPotentialPartitions,
    const std::map<TopicPartitionAssignment, std::vector<std::string>> &partition2AllPotentialConsumers,
    std::map<TopicPartitionAssignment, std::string> &currentPartitionConsumer)
{
    bool reassignmentPerformed = false;
    bool modified = false;
    do
    {
        modified = false;
        for (auto &partition : reassignablePartitions)
        {
            if (isBalanced(currentAssignment, consumer2AllPotentialPartitions))
                break;
            if (partition2AllPotentialConsumers.at(partition).size() <= 1)
            {
                // Log
                continue;
            }
            std::string consumer = currentPartitionConsumer[partition];
            if (consumer.empty())
            {
                // Log
                continue;
            }
            if (prevAssignment.count(partition))
            {
                auto &prev = prevAssignment.at(partition);
                if (static_cast<int>(currentAssignment[consumer].size()) >
                    static_cast<int>(currentAssignment[prev.m_member_id].size()) + 1)
                {
                    sortedCurrentSubscriptions = reassignPartition(partition, currentAssignment,
                                                                   sortedCurrentSubscriptions,
                                                                   currentPartitionConsumer,
                                                                   prev.m_member_id);
                    reassignmentPerformed = true;
                    modified = true;
                    continue;
                }
            }
            for (auto &otherConsumer : partition2AllPotentialConsumers.at(partition))
            {
                if (static_cast<int>(currentAssignment[consumer].size()) >
                    static_cast<int>(currentAssignment[otherConsumer].size()) + 1)
                {
                    sortedCurrentSubscriptions = reassignPartitionToNewConsumer(
                        partition, currentAssignment, sortedCurrentSubscriptions,
                        currentPartitionConsumer, consumer2AllPotentialPartitions);
                    reassignmentPerformed = true;
                    modified = true;
                    break;
                }
            }
        }
    } while (modified);
    return reassignmentPerformed;
}

std::vector<std::string> stickyBalanceStrategy::reassignPartitionToNewConsumer(
    const TopicPartitionAssignment &partition,
    std::map<std::string, std::vector<TopicPartitionAssignment>> &currentAssignment,
    std::vector<std::string> &sortedCurrentSubscriptions,
    std::map<TopicPartitionAssignment, std::string> &currentPartitionConsumer,
    const std::map<std::string, std::vector<TopicPartitionAssignment>> &consumer2AllPotentialPartitions)
{
    for (auto &anotherConsumer : sortedCurrentSubscriptions)
    {
        if (memberAssignmentsIncludeTopicPartition(consumer2AllPotentialPartitions.at(anotherConsumer), partition))
        {
            return reassignPartition(partition, currentAssignment, sortedCurrentSubscriptions,
                                     currentPartitionConsumer, anotherConsumer);
        }
    }
    return sortedCurrentSubscriptions;
}

std::vector<std::string> stickyBalanceStrategy::reassignPartition(
    const TopicPartitionAssignment &partition,
    std::map<std::string, std::vector<TopicPartitionAssignment>> &currentAssignment,
    std::vector<std::string> &sortedCurrentSubscriptions,
    std::map<TopicPartitionAssignment, std::string> &currentPartitionConsumer,
    const std::string &newConsumer)
{
    std::string consumer = currentPartitionConsumer[partition];
    TopicPartitionAssignment partitionToBeMoved =
        movements.getTheActualPartitionToBeMoved(partition, consumer, newConsumer);
    return processPartitionMovement(partitionToBeMoved, newConsumer, currentAssignment,
                                    sortedCurrentSubscriptions, currentPartitionConsumer);
}

std::vector<std::string> stickyBalanceStrategy::processPartitionMovement(
    const TopicPartitionAssignment &partition,
    const std::string &newConsumer,
    std::map<std::string, std::vector<TopicPartitionAssignment>> &currentAssignment,
    std::vector<std::string> &sortedCurrentSubscriptions,
    std::map<TopicPartitionAssignment, std::string> &currentPartitionConsumer)
{
    std::string oldConsumer = currentPartitionConsumer[partition];
    movements.movePartition(partition, oldConsumer, newConsumer);
    currentAssignment[oldConsumer] = removeTopicPartitionFromMemberAssignments(
        currentAssignment[oldConsumer], partition);
    currentAssignment[newConsumer].push_back(partition);
    currentPartitionConsumer[partition] = newConsumer;
    return sortMemberIDsByPartitionAssignments(currentAssignment);
}

// roundRobinBalancer implementations

int roundRobinBalancer::Plan(
    const std::map<std::string, ConsumerGroupMemberMetadata> &memberAndMetadata,
    const std::map<std::string, std::vector<int32_t>> &topics,
    BalanceStrategyPlan &plan)
{
    if (memberAndMetadata.empty())
    {
        return -1; // error code - no members to assign to
    }

    if (topics.empty())
    {
        plan.clear();
        return 0; // no topics to assign, return empty plan
    }

    std::vector<topicAndPartition> topicPartitions;
    for (auto &[topic, partitions] : topics)
    {
        for (int32_t p : partitions)
        {
            topicPartitions.push_back({topic, p});
        }
    }
    std::sort(topicPartitions.begin(), topicPartitions.end());

    std::vector<memberAndTopic> members;
    for (auto &[memberID, meta] : memberAndMetadata)
    {
        memberAndTopic m;
        m.memberID = memberID;
        for (auto &t : meta.m_topics)
        {
            m.topics.insert(t);
        }
        members.push_back(m);
    }
    std::sort(members.begin(), members.end(),
              [](const memberAndTopic &a, const memberAndTopic &b)
              {
                  return a.memberID < b.memberID;
              });

    plan.clear();
    size_t i = 0;
    size_t n = members.size();
    for (auto &tp : topicPartitions)
    {
        auto &m = members[i % n];
        size_t attempts = 0;
        while (!m.hasTopic(tp.topic))
        {
            i++;
            m = members[i % n];
            attempts++;
            if (attempts >= n)
            {
                break; // No member is subscribed to this topic, skip it
            }
        }
        if (m.hasTopic(tp.topic))
        {
            AddToPlan(plan, m.memberID, tp.topic, {tp.partition});
        }
        i++;
    }
    return 0;
}

int roundRobinBalancer::AssignmentData(
    const std::string &memberID,
    const std::map<std::string, std::vector<int32_t>> &topics,
    int32_t generationID,
    std::string &data)
{
    data.clear();
    return 0;
}