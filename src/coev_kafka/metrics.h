#pragma once

#include <string>
#include <map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <mutex>
#include <shared_mutex>

struct Broker;

namespace metrics
{
    struct IMetric
    {
        virtual int Mark(int) = 0;
        virtual void Update(int) = 0;
        ~IMetric() = default;
    };
    struct Meter : IMetric
    {
        Meter() = default;
        int Mark(int)
        {
            return 0;
        }
        void Update(int)
        {
        }
    };
    struct Histogram : IMetric
    {
        std::shared_ptr<IMetric> m_sample;
        Histogram() = default;
        Histogram(std::shared_ptr<IMetric> sample) : m_sample(sample)
        {
        }
        int Mark(int)
        {
            return 0;
        }
        void Update(int)
        {
        }
    };
    struct Counter : IMetric
    {
        void Inc(int)
        {
        }
        int Mark(int)
        {
            return 0;
        }
        void Update(int)
        {
        }
    };
    struct Sample : IMetric
    {
        int Mark(int)
        {
            return 0;
        }
        void Update(int)
        {
        }
    };
    struct Registry
    {
        virtual void Each(std::function<void(const std::string &, std::shared_ptr<IMetric>)> fn) = 0;
        virtual std::shared_ptr<IMetric> Get(const std::string &name) = 0;
        virtual std::shared_ptr<IMetric> GetOrRegister(const std::string &name, std::shared_ptr<IMetric> metric) = 0;
        virtual int Register(const std::string &name, std::shared_ptr<IMetric> metric) = 0;
        virtual void RunHealthChecks() = 0;
        virtual std::map<std::string, std::map<std::string, std::shared_ptr<IMetric>>> GetAll() = 0;
        virtual void Unregister(const std::string &name) = 0;
        virtual void UnregisterAll() = 0;
    };

    std::shared_ptr<Meter> GetOrRegisterMeter(const std::string &name, std::shared_ptr<Registry> r);

    std::shared_ptr<Sample> NewExpDecaySample(int, double);
    std::shared_ptr<Histogram> NewHistogram(std::shared_ptr<Sample>);
    std::shared_ptr<Registry> NewRegistry();
    std::shared_ptr<Histogram> GetOrRegisterHistogram(const std::string &name, std::shared_ptr<Registry> r);
    std::string GetMetricNameForBroker(const std::string &name, std::shared_ptr<Broker> broker);
    std::string GetMetricNameForTopic(const std::string &name, const std::string &topic);
    std::shared_ptr<Meter> GetOrRegisterTopicMeter(const std::string &name, const std::string &topic, std::shared_ptr<Registry> r);
    std::shared_ptr<Histogram> GetOrRegisterTopicHistogram(const std::string &name, const std::string &topic, std::shared_ptr<Registry> r);
    std::shared_ptr<Counter> GetOrRegisterCounter(const std::string &name, std::shared_ptr<Registry> r);

    struct CleanupRegistry : Registry
    {
        CleanupRegistry(std::shared_ptr<Registry> parent);
        void Each(std::function<void(const std::string &, std::shared_ptr<IMetric>)> fn);
        std::shared_ptr<IMetric> Get(const std::string &name);
        std::shared_ptr<IMetric> GetOrRegister(const std::string &name, std::shared_ptr<IMetric> metric);
        int Register(const std::string &name, std::shared_ptr<IMetric> metric);
        void RunHealthChecks();
        std::map<std::string, std::map<std::string, std::shared_ptr<IMetric>>> GetAll();
        void Unregister(const std::string &name);
        void UnregisterAll();

        std::shared_ptr<Registry> parent;
        std::unordered_set<std::string> metrics;
        std::shared_mutex mutex;
    };

    std::shared_ptr<Registry> NewCleanupRegistry(std::shared_ptr<Registry> parent);

}