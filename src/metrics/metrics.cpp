#include "protoactor/metrics/metrics.h"
#include <mutex>
#include <unordered_map>
#include <memory>

namespace protoactor {
namespace metrics {

// Default no-op implementations
class NoOpCounter : public Counter {
public:
    void Increment(double delta) override {}
    void Add(double value) override {}
};

class NoOpGauge : public Gauge {
public:
    void Set(double value) override {}
    void Increment(double delta) override {}
    void Decrement(double delta) override {}
};

class NoOpHistogram : public Histogram {
public:
    void Record(double value) override {}
};

class NoOpTimer : public Timer {
public:
    void Record(std::chrono::milliseconds duration) override {}
    std::shared_ptr<void> Start() override { return nullptr; }
};

// Default no-op provider
class NoOpMetricsProvider : public MetricsProvider {
public:
    std::shared_ptr<Counter> NewCounter(const std::string& name, const std::string& description) override {
        return std::make_shared<NoOpCounter>();
    }
    
    std::shared_ptr<Gauge> NewGauge(const std::string& name, const std::string& description) override {
        return std::make_shared<NoOpGauge>();
    }
    
    std::shared_ptr<Histogram> NewHistogram(const std::string& name, const std::string& description) override {
        return std::make_shared<NoOpHistogram>();
    }
    
    std::shared_ptr<Timer> NewTimer(const std::string& name, const std::string& description) override {
        return std::make_shared<NoOpTimer>();
    }
};

// Static provider instance
std::shared_ptr<MetricsProvider> Metrics::provider_ = std::make_shared<NoOpMetricsProvider>();
static std::mutex provider_mutex_;

void Metrics::SetProvider(std::shared_ptr<MetricsProvider> provider) {
    std::lock_guard<std::mutex> lock(provider_mutex_);
    provider_ = provider ? provider : std::make_shared<NoOpMetricsProvider>();
}

std::shared_ptr<MetricsProvider> Metrics::GetProvider() {
    std::lock_guard<std::mutex> lock(provider_mutex_);
    return provider_;
}

std::shared_ptr<Counter> Metrics::NewCounter(const std::string& name, const std::string& description) {
    return GetProvider()->NewCounter(name, description);
}

std::shared_ptr<Gauge> Metrics::NewGauge(const std::string& name, const std::string& description) {
    return GetProvider()->NewGauge(name, description);
}

std::shared_ptr<Histogram> Metrics::NewHistogram(const std::string& name, const std::string& description) {
    return GetProvider()->NewHistogram(name, description);
}

std::shared_ptr<Timer> Metrics::NewTimer(const std::string& name, const std::string& description) {
    return GetProvider()->NewTimer(name, description);
}

// ActorMetrics implementation
ActorMetrics::ActorMetrics(std::shared_ptr<MetricsProvider> provider) {
    actor_spawn_count = provider->NewCounter("actor_spawn_count", "Number of actors spawned");
    actor_stopped_count = provider->NewCounter("actor_stopped_count", "Number of actors stopped");
    actor_restarted_count = provider->NewCounter("actor_restarted_count", "Number of actors restarted");
    actor_mailbox_size = provider->NewHistogram("actor_mailbox_size", "Actor mailbox size");
    actor_message_duration = provider->NewTimer("actor_message_duration", "Actor message processing duration");
}

} // namespace metrics
} // namespace protoactor
