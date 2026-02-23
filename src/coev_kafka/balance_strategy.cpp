#include "balance_strategy.h"
#include "topic_type.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <set>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include "balance_strategy.h"
#include "topic_type.h"

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

        for (auto &[topic, member_ids] : mbt)
        {
            auto it = topics.find(topic);
            if (it == topics.end())
            {
                continue; // Skip topics that don't exist in the topics map
            }
            auto uniqueMembers = uniq(member_ids);
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
    auto core = [](BalanceStrategyPlan &plan, std::vector<std::string> &member_ids, const std::string &topic, const std::vector<int32_t> &partitions)
    {
        int partitionsPerConsumer = static_cast<int>(partitions.size()) / static_cast<int>(member_ids.size());
        int consumersWithExtraPartition = static_cast<int>(partitions.size()) % static_cast<int>(member_ids.size());
        std::sort(member_ids.begin(), member_ids.end());
        for (size_t i = 0; i < member_ids.size(); ++i)
        {
            int min = static_cast<int>(i) * partitionsPerConsumer + static_cast<int>(std::min(static_cast<double>(consumersWithExtraPartition), static_cast<double>(i)));
            int extra = (i < static_cast<size_t>(consumersWithExtraPartition)) ? 1 : 0;
            int max = min + partitionsPerConsumer + extra;
            std::vector<int32_t> slice(partitions.begin() + min, partitions.begin() + max);
            AddToPlan(plan, member_ids[i], topic, slice);
        }
    };
    return std::make_shared<balanceStrategy>(core, RangeBalanceStrategyName);
}

std::shared_ptr<BalanceStrategy> BalanceStrategyRange = NewBalanceStrategyRange();

struct consumer_movement
{
    std::string SrcMemberID;
    std::string DstMemberID;
    bool operator==(const consumer_movement &other) const
    {
        return SrcMemberID == other.SrcMemberID && DstMemberID == other.DstMemberID;
    }
    bool operator<(const consumer_movement &other) const
    {
        return SrcMemberID < other.SrcMemberID || (SrcMemberID == other.SrcMemberID && DstMemberID < other.DstMemberID);
    }
};

struct PartitionMovements
{
    std::map<std::string, std::map<consumer_movement, std::map<topic_t, bool>>> PartitionMovementsByTopic;
    std::map<topic_t, consumer_movement> Movements;

    void remove_movement_record_of_partition(const topic_t &partition, consumer_movement &outPair);
    void add_partition_movement_record(const topic_t &partition, const consumer_movement &pair);
    void move_partition(const topic_t &partition, const std::string &oldConsumer, const std::string &newConsumer);
    topic_t get_the_actual_partition_to_be_moved(const topic_t &partition, const std::string &oldConsumer, const std::string &newConsumer);
    std::pair<std::vector<std::string>, bool> is_linked(const std::string &src, const std::string &dst, std::vector<consumer_movement> pairs, std::vector<std::string> currentPath);
    bool in(const std::vector<std::string> &cycle, const std::vector<std::vector<std::string>> &cycles);
    bool has_cycles(const std::vector<consumer_movement> &pairs);
    bool is_sticky();
};

struct StickyBalanceStrategy : BalanceStrategy
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
    PartitionMovements movements;

    void balance(std::map<std::string, std::vector<topic_t>> &currentAssignment,
                 const std::map<topic_t, consumerGenerationPair> &prevAssignment,
                 std::vector<topic_t> &sortedPartitions,
                 std::vector<topic_t> &unassignedPartitions,
                 std::vector<std::string> &sortedCurrentSubscriptions,
                 const std::map<std::string, std::vector<topic_t>> &consumer2AllPotentialPartitions,
                 const std::map<topic_t, std::vector<std::string>> &partition2AllPotentialConsumers,
                 std::map<topic_t, std::string> &currentPartitionConsumer);

    bool performReassignments(std::vector<topic_t> &reassignablePartitions,
                              std::map<std::string, std::vector<topic_t>> &currentAssignment,
                              const std::map<topic_t, consumerGenerationPair> &prevAssignment,
                              std::vector<std::string> &sortedCurrentSubscriptions,
                              const std::map<std::string, std::vector<topic_t>> &consumer2AllPotentialPartitions,
                              const std::map<topic_t, std::vector<std::string>> &partition2AllPotentialConsumers,
                              std::map<topic_t, std::string> &currentPartitionConsumer);

    std::vector<std::string> reassignPartitionToNewConsumer(const topic_t &partition,
                                                            std::map<std::string, std::vector<topic_t>> &currentAssignment,
                                                            std::vector<std::string> &sortedCurrentSubscriptions,
                                                            std::map<topic_t, std::string> &currentPartitionConsumer,
                                                            const std::map<std::string, std::vector<topic_t>> &consumer2AllPotentialPartitions);

    std::vector<std::string> reassignPartition(const topic_t &partition,
                                               std::map<std::string, std::vector<topic_t>> &currentAssignment,
                                               std::vector<std::string> &sortedCurrentSubscriptions,
                                               std::map<topic_t, std::string> &currentPartitionConsumer,
                                               const std::string &newConsumer);

    std::vector<std::string> processPartitionMovement(const topic_t &partition,
                                                      const std::string &newConsumer,
                                                      std::map<std::string, std::vector<topic_t>> &currentAssignment,
                                                      std::vector<std::string> &sortedCurrentSubscriptions,
                                                      std::map<topic_t, std::string> &currentPartitionConsumer);
};

std::shared_ptr<BalanceStrategy> NewBalanceStrategySticky()
{
    return std::make_shared<StickyBalanceStrategy>();
}

std::shared_ptr<BalanceStrategy> BalanceStrategySticky = NewBalanceStrategySticky();

struct TopicAndPartition
{
    std::string topic;
    int32_t partition;
    std::string comparedValue() const
    {
        return topic + "-" + std::to_string(partition);
    }
    bool operator<(const TopicAndPartition &other) const
    {
        return comparedValue() < other.comparedValue();
    }
};

struct MemberAndTopic
{
    std::unordered_set<std::string> topics;
    std::string memberID;
    bool hasTopic(const std::string &topic) const
    {
        return topics.count(topic) > 0;
    }
};

class RoundRobinBalancer : public BalanceStrategy
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
    return std::make_shared<RoundRobinBalancer>();
}

std::shared_ptr<BalanceStrategy> BalanceStrategyRoundRobin = NewBalanceStrategyRoundRobin();

int getBalanceScore(const std::map<std::string, std::vector<topic_t>> &assignment)
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

bool isBalanced(const std::map<std::string, std::vector<topic_t>> &currentAssignment,
                const std::map<std::string, std::vector<topic_t>> &allSubscriptions)
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

    std::map<topic_t, std::string> allPartitions_;
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
    const std::map<std::string, std::vector<topic_t>> &currentAssignment,
    const std::map<std::string, std::vector<topic_t>> &consumer2AllPotentialPartitions,
    const std::map<topic_t, std::vector<std::string>> &partition2AllPotentialConsumers)
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
    const topic_t &partition,
    const std::map<topic_t, std::vector<std::string>> &partition2AllPotentialConsumers)
{
    auto it = partition2AllPotentialConsumers.find(partition);
    return it != partition2AllPotentialConsumers.end() && it->second.size() >= 2;
}

std::vector<std::string> assignPartition(
    const topic_t &partition,
    std::vector<std::string> sortedCurrentSubscriptions,
    std::map<std::string, std::vector<topic_t>> &currentAssignment,
    const std::map<std::string, std::vector<topic_t>> &consumer2AllPotentialPartitions,
    std::map<topic_t, std::string> &currentPartitionConsumer)
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

std::vector<topic_t> removeTopicPartitionFromMemberAssignments(
    std::vector<topic_t> assignments,
    const topic_t &topic)
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
    const std::vector<topic_t> &assignments,
    const topic_t &topic)
{
    for (auto &a : assignments)
    {
        if (a == topic)
            return true;
    }
    return false;
}

std::vector<std::string> sortMemberIDsByPartitionAssignments(
    const std::map<std::string, std::vector<topic_t>> &assignments)
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

std::vector<topic_t> sortPartitionsByPotentialConsumerAssignments(
    const std::map<topic_t, std::vector<std::string>> &partition2AllPotentialConsumers)
{
    std::vector<topic_t> sortedPartitionIDs;
    for (auto &[partition, _] : partition2AllPotentialConsumers)
    {
        sortedPartitionIDs.push_back(partition);
    }
    std::sort(sortedPartitionIDs.begin(), sortedPartitionIDs.end(),
              [&](const topic_t &a, const topic_t &b)
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

std::map<std::string, std::vector<topic_t>> deepCopyAssignment(
    const std::map<std::string, std::vector<topic_t>> &assignment)
{
    std::map<std::string, std::vector<topic_t>> m;
    for (auto &[memberID, subscriptions] : assignment)
    {
        m[memberID] = subscriptions;
    }
    return m;
}

bool areSubscriptionsIdentical(
    const std::map<topic_t, std::vector<std::string>> &partition2AllPotentialConsumers,
    const std::map<std::string, std::vector<topic_t>> &consumer2AllPotentialPartitions)
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

    std::map<topic_t, int> curPartitions_;
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
        std::map<topic_t, int> yMap_;
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

int prepopulateCurrentAssignments(const std::map<std::string, ConsumerGroupMemberMetadata> &members, std::map<std::string, std::vector<topic_t>> &currentAssignment, std::map<topic_t, consumerGenerationPair> &prevAssignment)
{

    std::map<topic_t, std::map<int, std::string>> sortedPartitionConsumersByGeneration;

    for (auto &it : members)
    {
        auto &memberID = it.first;
        auto &meta = it.second;

        std::vector<topic_t> userPartitions;
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

void PartitionMovements::remove_movement_record_of_partition(const topic_t &partition, consumer_movement &outPair)
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

void PartitionMovements::add_partition_movement_record(const topic_t &partition, const consumer_movement &pair)
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

void PartitionMovements::move_partition(const topic_t &partition, const std::string &oldConsumer, const std::string &newConsumer)
{
    consumer_movement pair{oldConsumer, newConsumer};
    if (Movements.count(partition))
    {
        consumer_movement existingPair;
        remove_movement_record_of_partition(partition, existingPair);
        if (existingPair.DstMemberID != oldConsumer)
        {
            // Log mismatch
        }
        if (existingPair.SrcMemberID != newConsumer)
        {
            add_partition_movement_record(partition, consumer_movement{existingPair.SrcMemberID, newConsumer});
        }
    }
    else
    {
        add_partition_movement_record(partition, pair);
    }
}

topic_t PartitionMovements::get_the_actual_partition_to_be_moved(
    const topic_t &partition,
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
    consumer_movement reversePair{newConsumer, actualOld};
    if (PartitionMovementsByTopic[partition.m_topic].count(reversePair) == 0)
    {
        return partition;
    }
    auto &revMap = PartitionMovementsByTopic[partition.m_topic][reversePair];
    return revMap.begin()->first;
}

std::pair<std::vector<std::string>, bool> PartitionMovements::is_linked(
    const std::string &src,
    const std::string &dst,
    std::vector<consumer_movement> pairs,
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
        std::vector<consumer_movement> reducedSet;
        for (size_t j = 0; j < pairs.size(); ++j)
        {
            if (j != i)
                reducedSet.push_back(pairs[j]);
        }
        currentPath.push_back(src);
        return is_linked(pairs[i].DstMemberID, dst, reducedSet, currentPath);
    }
    return {currentPath, false};
}

bool PartitionMovements::in(const std::vector<std::string> &cycle, const std::vector<std::vector<std::string>> &cycles)
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

bool PartitionMovements::has_cycles(const std::vector<consumer_movement> &pairs)
{
    std::vector<std::vector<std::string>> cycles;
    for (size_t i = 0; i < pairs.size(); ++i)
    {
        std::vector<consumer_movement> reducedPairs;
        for (size_t j = 0; j < pairs.size(); ++j)
        {
            if (j != i)
                reducedPairs.push_back(pairs[j]);
        }
        auto [path, linked] = is_linked(pairs[i].DstMemberID, pairs[i].SrcMemberID, reducedPairs, {pairs[i].SrcMemberID});
        if (linked && !in(path, cycles))
        {
            cycles.push_back(path);
            if (path.size() == 3)
                return true;
        }
    }
    return false;
}

bool PartitionMovements::is_sticky()
{
    for (auto &[topic, movements] : PartitionMovementsByTopic)
    {
        std::vector<consumer_movement> movementPairs;
        for (auto &[pair, _] : movements)
        {
            movementPairs.push_back(pair);
        }
        if (has_cycles(movementPairs))
        {
            return false;
        }
    }
    return true;
}

int StickyBalanceStrategy::Plan(const std::map<std::string, ConsumerGroupMemberMetadata> &members, const std::map<std::string, std::vector<int32_t>> &topics, BalanceStrategyPlan &plan)
{
    // Handle edge case: no topics to assign
    if (topics.empty())
    {
        plan.clear();
        return 0;
    }

    movements = PartitionMovements{};
    std::map<std::string, std::vector<topic_t>> currentAssignment;
    std::map<topic_t, consumerGenerationPair> prevAssignment;
    auto err = prepopulateCurrentAssignments(members, currentAssignment, prevAssignment);
    bool isFreshAssignment = currentAssignment.empty();

    std::map<topic_t, std::vector<std::string>> partition2AllPotentialConsumers;
    for (auto &[topic, partitions] : topics)
    {
        for (int32_t p : partitions)
        {
            topic_t tpa{topic, p};
            partition2AllPotentialConsumers[tpa] = {};
        }
    }

    std::map<std::string, std::vector<topic_t>> consumer2AllPotentialPartitions;
    for (auto &it : members)
    {
        auto &memberID = it.first;
        auto &meta = it.second;
        std::vector<topic_t> list;
        for (auto &topicSubscription : meta.m_topics)
        {
            auto topicIt = topics.find(topicSubscription);
            if (topicIt != topics.end())
            {
                for (int32_t p : topicIt->second)
                {
                    topic_t tpa(topicSubscription, p);
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

    std::map<topic_t, std::string> currentPartitionConsumers;
    std::map<topic_t, bool> unvisitedPartitions;
    for (auto &[partition, _] : partition2AllPotentialConsumers)
    {
        unvisitedPartitions[partition] = true;
    }

    std::vector<topic_t> unassignedPartitions;
    for (auto &it : currentAssignment)
    {
        auto &memberID = it.first;
        auto &partitions = it.second;
        std::vector<topic_t> keepPartitions;
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

    std::vector<topic_t> sortedPartitions = sortPartitionsHelper(
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

int StickyBalanceStrategy::AssignmentData(
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

void StickyBalanceStrategy::balance(
    std::map<std::string, std::vector<topic_t>> &currentAssignment,
    const std::map<topic_t, consumerGenerationPair> &prevAssignment,
    std::vector<topic_t> &sortedPartitions,
    std::vector<topic_t> &unassignedPartitions,
    std::vector<std::string> &sortedCurrentSubscriptions,
    const std::map<std::string, std::vector<topic_t>> &consumer2AllPotentialPartitions,
    const std::map<topic_t, std::vector<std::string>> &partition2AllPotentialConsumers,
    std::map<topic_t, std::string> &currentPartitionConsumer)
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

    std::map<std::string, std::vector<topic_t>> fixedAssignments;
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

bool StickyBalanceStrategy::performReassignments(
    std::vector<topic_t> &reassignablePartitions,
    std::map<std::string, std::vector<topic_t>> &currentAssignment,
    const std::map<topic_t, consumerGenerationPair> &prevAssignment,
    std::vector<std::string> &sortedCurrentSubscriptions,
    const std::map<std::string, std::vector<topic_t>> &consumer2AllPotentialPartitions,
    const std::map<topic_t, std::vector<std::string>> &partition2AllPotentialConsumers,
    std::map<topic_t, std::string> &currentPartitionConsumer)
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

std::vector<std::string> StickyBalanceStrategy::reassignPartitionToNewConsumer(
    const topic_t &partition,
    std::map<std::string, std::vector<topic_t>> &currentAssignment,
    std::vector<std::string> &sortedCurrentSubscriptions,
    std::map<topic_t, std::string> &currentPartitionConsumer,
    const std::map<std::string, std::vector<topic_t>> &consumer2AllPotentialPartitions)
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

std::vector<std::string> StickyBalanceStrategy::reassignPartition(
    const topic_t &partition,
    std::map<std::string, std::vector<topic_t>> &currentAssignment,
    std::vector<std::string> &sortedCurrentSubscriptions,
    std::map<topic_t, std::string> &currentPartitionConsumer,
    const std::string &newConsumer)
{
    std::string consumer = currentPartitionConsumer[partition];
    topic_t partitionToBeMoved =
        movements.get_the_actual_partition_to_be_moved(partition, consumer, newConsumer);
    return processPartitionMovement(partitionToBeMoved, newConsumer, currentAssignment,
                                    sortedCurrentSubscriptions, currentPartitionConsumer);
}

std::vector<std::string> StickyBalanceStrategy::processPartitionMovement(
    const topic_t &partition,
    const std::string &newConsumer,
    std::map<std::string, std::vector<topic_t>> &currentAssignment,
    std::vector<std::string> &sortedCurrentSubscriptions,
    std::map<topic_t, std::string> &currentPartitionConsumer)
{
    std::string oldConsumer = currentPartitionConsumer[partition];
    movements.move_partition(partition, oldConsumer, newConsumer);
    currentAssignment[oldConsumer] = removeTopicPartitionFromMemberAssignments(
        currentAssignment[oldConsumer], partition);
    currentAssignment[newConsumer].push_back(partition);
    currentPartitionConsumer[partition] = newConsumer;
    return sortMemberIDsByPartitionAssignments(currentAssignment);
}

// RoundRobinBalancer implementations

int RoundRobinBalancer::Plan(
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

    std::vector<TopicAndPartition> topicPartitions;
    for (auto &[topic, partitions] : topics)
    {
        for (int32_t p : partitions)
        {
            topicPartitions.push_back({topic, p});
        }
    }
    std::sort(topicPartitions.begin(), topicPartitions.end());

    std::vector<MemberAndTopic> members;
    for (auto &[memberID, meta] : memberAndMetadata)
    {
        MemberAndTopic m;
        m.memberID = memberID;
        for (auto &t : meta.m_topics)
        {
            m.topics.insert(t);
        }
        members.push_back(m);
    }
    std::sort(members.begin(), members.end(),
              [](const MemberAndTopic &a, const MemberAndTopic &b)
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

int RoundRobinBalancer::AssignmentData(
    const std::string &memberID,
    const std::map<std::string, std::vector<int32_t>> &topics,
    int32_t generationID,
    std::string &data)
{
    data.clear();
    return 0;
}