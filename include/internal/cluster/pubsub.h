#ifndef PROTOACTOR_CLUSTER_PUBSUB_H
#define PROTOACTOR_CLUSTER_PUBSUB_H

#include "external/pid.h"
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>

namespace protoactor {
namespace cluster {

// Forward declaration
class Cluster;

/**
 * @brief PubSub provides publish-subscribe functionality across the cluster.
 */
class PubSub {
public:
    explicit PubSub(std::shared_ptr<Cluster> cluster);
    ~PubSub();
    
    /**
     * @brief Start the PubSub subsystem.
     */
    void Start();
    
    /**
     * @brief Stop the PubSub subsystem.
     */
    void Stop();
    
    /**
     * @brief Publish a message to a topic.
     * @param topic Topic name
     * @param message Message to publish
     */
    void Publish(const std::string& topic, std::shared_ptr<void> message);
    
    /**
     * @brief Subscribe to a topic.
     * @param topic Topic name
     * @param subscriber Subscriber PID
     */
    void Subscribe(const std::string& topic, std::shared_ptr<PID> subscriber);
    
    /**
     * @brief Unsubscribe from a topic.
     * @param topic Topic name
     * @param subscriber Subscriber PID
     */
    void Unsubscribe(const std::string& topic, std::shared_ptr<PID> subscriber);

private:
    std::shared_ptr<Cluster> cluster_;
    std::atomic<bool> running_;
    std::shared_ptr<PID> delivery_actor_;
    std::unordered_map<std::string, std::vector<std::shared_ptr<PID>>> subscriptions_;
    std::mutex subscriptions_mutex_;
};

} // namespace cluster
} // namespace protoactor

#endif // PROTOACTOR_CLUSTER_PUBSUB_H
