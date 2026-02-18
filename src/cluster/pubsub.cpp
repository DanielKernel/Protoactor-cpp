#include "protoactor/internal/cluster/pubsub.h"
#include "protoactor/cluster/cluster.h"
#include "protoactor/internal/cluster/pubsub_delivery.h"
#include "protoactor/actor_system.h"
#include "protoactor/props.h"
#include "protoactor/messages.h"
#include <algorithm>
#include <unordered_map>

namespace protoactor {
namespace cluster {

PubSub::PubSub(std::shared_ptr<Cluster> cluster)
    : cluster_(std::move(cluster)), running_(false) {
}

PubSub::~PubSub() {
    Stop();
}

void PubSub::Start() {
    if (running_.load(std::memory_order_acquire)) {
        return;
    }
    
    running_.store(true, std::memory_order_release);
    
    // Spawn PubSubMemberDeliveryActor
    auto props = Props::FromProducer([this]() {
        return std::make_shared<PubSubMemberDeliveryActor>(
            std::chrono::milliseconds(5000), // 5 second timeout
            cluster_);
    });
    
    auto [pid, err] = cluster_->GetActorSystem()->GetRoot()->SpawnNamed(props, "$pubsub-delivery");
    if (!err) {
        delivery_actor_ = pid;
    }
}

void PubSub::Stop() {
    if (!running_.exchange(false, std::memory_order_acq_rel)) {
        return; // Already stopped
    }
    
    if (delivery_actor_) {
        cluster_->GetActorSystem()->GetRoot()->StopFuture(delivery_actor_)->Wait();
        delivery_actor_ = nullptr;
    }
    
    // Clear subscriptions
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    subscriptions_.clear();
}

void PubSub::Publish(const std::string& topic, std::shared_ptr<void> message) {
    if (!running_.load(std::memory_order_acquire)) {
        return;
    }
    
    // Get subscribers for topic
    std::vector<std::shared_ptr<PID>> subscribers;
    {
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        auto it = subscriptions_.find(topic);
        if (it != subscriptions_.end()) {
            subscribers = it->second;
        }
    }
    
    if (subscribers.empty()) {
        return;
    }
    
    // Create envelope
    auto envelope = std::make_shared<MessageEnvelope>(nullptr, message, nullptr);
    
    // Create delivery batch request
    auto batch = std::make_shared<DeliverBatchRequest>();
    batch->topic = topic;
    batch->envelopes.push_back(envelope);
    batch->subscribers = subscribers;
    
    // Send to delivery actor
    if (delivery_actor_) {
        cluster_->GetActorSystem()->GetRoot()->Send(delivery_actor_, batch);
    }
}

void PubSub::Subscribe(const std::string& topic, std::shared_ptr<PID> subscriber) {
    if (!running_.load(std::memory_order_acquire) || !subscriber) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    auto& subs = subscriptions_[topic];
    
    // Check if already subscribed
    auto it = std::find(subs.begin(), subs.end(), subscriber);
    if (it == subs.end()) {
        subs.push_back(subscriber);
    }
}

void PubSub::Unsubscribe(const std::string& topic, std::shared_ptr<PID> subscriber) {
    if (!running_.load(std::memory_order_acquire) || !subscriber) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    auto it = subscriptions_.find(topic);
    if (it != subscriptions_.end()) {
        auto& subs = it->second;
        subs.erase(
            std::remove(subs.begin(), subs.end(), subscriber),
            subs.end());
        
        if (subs.empty()) {
            subscriptions_.erase(it);
        }
    }
}

} // namespace cluster
} // namespace protoactor
