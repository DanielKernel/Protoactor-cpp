#ifndef PROTOACTOR_METRICS_METRICS_H
#define PROTOACTOR_METRICS_METRICS_H

#include <string>
#include <memory>
#include <functional>
#include <chrono>

namespace protoactor {
namespace metrics {

/**
 * @brief Counter metric.
 */
class Counter {
public:
    virtual ~Counter() = default;
    virtual void Increment(double delta = 1.0) = 0;
    virtual void Add(double value) = 0;
};

/**
 * @brief Gauge metric.
 */
class Gauge {
public:
    virtual ~Gauge() = default;
    virtual void Set(double value) = 0;
    virtual void Increment(double delta = 1.0) = 0;
    virtual void Decrement(double delta = 1.0) = 0;
};

/**
 * @brief Histogram metric.
 */
class Histogram {
public:
    virtual ~Histogram() = default;
    virtual void Record(double value) = 0;
};

/**
 * @brief Timer metric.
 */
class Timer {
public:
    virtual ~Timer() = default;
    virtual void Record(std::chrono::milliseconds duration) = 0;
    virtual std::shared_ptr<void> Start() = 0; // Returns a stop function
};

/**
 * @brief Metrics provider interface.
 */
class MetricsProvider {
public:
    virtual ~MetricsProvider() = default;
    
    virtual std::shared_ptr<Counter> NewCounter(const std::string& name, const std::string& description = "") = 0;
    virtual std::shared_ptr<Gauge> NewGauge(const std::string& name, const std::string& description = "") = 0;
    virtual std::shared_ptr<Histogram> NewHistogram(const std::string& name, const std::string& description = "") = 0;
    virtual std::shared_ptr<Timer> NewTimer(const std::string& name, const std::string& description = "") = 0;
};

/**
 * @brief Actor-specific metrics.
 */
class ActorMetrics {
public:
    std::shared_ptr<Counter> actor_spawn_count;
    std::shared_ptr<Counter> actor_stopped_count;
    std::shared_ptr<Counter> actor_restarted_count;
    std::shared_ptr<Histogram> actor_mailbox_size;
    std::shared_ptr<Timer> actor_message_duration;
    
    ActorMetrics(std::shared_ptr<MetricsProvider> provider);
};

/**
 * @brief Global metrics registry.
 */
class Metrics {
public:
    /**
     * @brief Set the metrics provider.
     * @param provider Metrics provider
     */
    static void SetProvider(std::shared_ptr<MetricsProvider> provider);
    
    /**
     * @brief Get the metrics provider.
     * @return Metrics provider
     */
    static std::shared_ptr<MetricsProvider> GetProvider();
    
    /**
     * @brief Create a counter.
     * @param name Counter name
     * @param description Counter description
     * @return Counter instance
     */
    static std::shared_ptr<Counter> NewCounter(const std::string& name, const std::string& description = "");
    
    /**
     * @brief Create a gauge.
     * @param name Gauge name
     * @param description Gauge description
     * @return Gauge instance
     */
    static std::shared_ptr<Gauge> NewGauge(const std::string& name, const std::string& description = "");
    
    /**
     * @brief Create a histogram.
     * @param name Histogram name
     * @param description Histogram description
     * @return Histogram instance
     */
    static std::shared_ptr<Histogram> NewHistogram(const std::string& name, const std::string& description = "");
    
    /**
     * @brief Create a timer.
     * @param name Timer name
     * @param description Timer description
     * @return Timer instance
     */
    static std::shared_ptr<Timer> NewTimer(const std::string& name, const std::string& description = "");

private:
    static std::shared_ptr<MetricsProvider> provider_;
};

} // namespace metrics
} // namespace protoactor

#endif // PROTOACTOR_METRICS_METRICS_H
