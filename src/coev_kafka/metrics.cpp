#include "metrics.h"
#include "broker.h"
#include <sstream>
#include <algorithm>

namespace metrics
{
    inline constexpr int metricsReservoirSize = 1028;
    inline constexpr double metricsAlphaFactor = 0.015;
    struct StandardRegistry : Registry
    {
        void Each(std::function<void(const std::string &, std::shared_ptr<IMetric>)> fn) {}
        std::shared_ptr<IMetric> Get(const std::string &name) { return nullptr; }
        std::shared_ptr<IMetric> GetOrRegister(const std::string &name, std::shared_ptr<IMetric> metric) { return metric; }
        int Register(const std::string &name, std::shared_ptr<IMetric> metric) { return 0; }
        void RunHealthChecks() {}
        std::map<std::string, std::map<std::string, std::shared_ptr<IMetric>>> GetAll() { return {}; }
        void Unregister(const std::string &name) {}
        void UnregisterAll() {}
    } DefaultRegistry;

    std::shared_ptr<Histogram> GetOrRegisterHistogram(const std::string &name, std::shared_ptr<Registry> r)
    {
        std::shared_ptr<Histogram> histogram = std::dynamic_pointer_cast<Histogram>(r->Get(name));
        if (!histogram)
        {
            auto sample = NewExpDecaySample(metricsReservoirSize, metricsAlphaFactor);
            histogram = NewHistogram(sample);
            r->GetOrRegister(name, histogram);
        }
        return histogram;
    }

    std::string GetMetricNameForBroker(const std::string &name, std::shared_ptr<Broker> broker)
    {
        std::ostringstream oss;
        oss << name << "-for-broker-" << broker->ID();
        return oss.str();
    }

    std::string GetMetricNameForTopic(const std::string &name, const std::string &topic)
    {
        std::string result = name + "-for-topic-" + topic;
        std::replace(result.begin(), result.end(), '.', '_');
        return result;
    }

    std::shared_ptr<Meter> GetOrRegisterTopicMeter(const std::string &name, const std::string &topic, std::shared_ptr<Registry> r)
    {
        std::string metricName = GetMetricNameForTopic(name, topic);
        return std::dynamic_pointer_cast<Meter>(r->GetOrRegister(metricName, nullptr));
    }

    std::shared_ptr<Histogram> GetOrRegisterTopicHistogram(const std::string &name, const std::string &topic, std::shared_ptr<Registry> r)
    {
        std::string metricName = GetMetricNameForTopic(name, topic);
        return GetOrRegisterHistogram(metricName, r);
    }

    CleanupRegistry::CleanupRegistry(std::shared_ptr<Registry> parent)
        : parent(std::move(parent)) {}

    void CleanupRegistry::Each(std::function<void(const std::string &, std::shared_ptr<IMetric>)> fn)
    {
        std::shared_lock<std::shared_mutex> lock(mutex);
        auto wrappedFn = [this, &fn](const std::string &name, std::shared_ptr<IMetric> iface)
        {
            if (metrics.find(name) != metrics.end())
            {
                fn(name, iface);
            }
        };
        parent->Each(wrappedFn);
    }

    std::shared_ptr<IMetric> CleanupRegistry::Get(const std::string &name)
    {
        std::shared_lock<std::shared_mutex> lock(mutex);
        if (metrics.find(name) != metrics.end())
        {
            return parent->Get(name);
        }
        return nullptr;
    }

    std::shared_ptr<IMetric> CleanupRegistry::GetOrRegister(const std::string &name, std::shared_ptr<IMetric> metric)
    {
        std::unique_lock<std::shared_mutex> lock(mutex);
        metrics.insert(name);
        return parent->GetOrRegister(name, metric);
    }

    int CleanupRegistry::Register(const std::string &name, std::shared_ptr<IMetric> metric)
    {
        std::unique_lock<std::shared_mutex> lock(mutex);
        metrics.insert(name);
        return parent->Register(name, metric);
    }

    void CleanupRegistry::RunHealthChecks()
    {
        parent->RunHealthChecks();
    }

    std::map<std::string, std::map<std::string, std::shared_ptr<IMetric>>> CleanupRegistry::GetAll()
    {
        return parent->GetAll();
    }

    void CleanupRegistry::Unregister(const std::string &name)
    {
        std::unique_lock<std::shared_mutex> lock(mutex);
        if (metrics.find(name) != metrics.end())
        {
            metrics.erase(name);
            parent->Unregister(name);
        }
    }

    void CleanupRegistry::UnregisterAll()
    {
        std::unique_lock<std::shared_mutex> lock(mutex);
        for (auto &name : metrics)
        {
            parent->Unregister(name);
        }
        metrics.clear();
    }

    std::shared_ptr<Registry> NewCleanupRegistry(std::shared_ptr<Registry> parent)
    {
        return std::make_shared<CleanupRegistry>(parent);
    }
    std::shared_ptr<Meter> GetOrRegisterMeter(const std::string &name, std::shared_ptr<Registry> r)
    {
        auto meter = std::make_shared<Meter>();
        r ? r->GetOrRegister(name, meter) : DefaultRegistry.GetOrRegister(name, meter);
        return meter;
    }
    std::shared_ptr<Counter> GetOrRegisterCounter(const std::string &name, std::shared_ptr<Registry> r)
    {
        auto counter = std::make_shared<Counter>();
        if (r)
            r->GetOrRegister(name, counter);
        else
            DefaultRegistry.GetOrRegister(name, counter);
        return counter;
    }
    std::shared_ptr<Sample> NewExpDecaySample(int, double)
    {
        return nullptr;
    }
    std::shared_ptr<Histogram> NewHistogram(std::shared_ptr<Sample>)
    {
        return nullptr;
    }
    std::shared_ptr<Registry> NewRegistry()
    {
        return std::make_shared<StandardRegistry>();
    }
}