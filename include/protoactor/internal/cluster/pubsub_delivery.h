#ifndef PROTOACTOR_CLUSTER_PUBSUB_DELIVERY_H
#define PROTOACTOR_CLUSTER_PUBSUB_DELIVERY_H

#include "protoactor/actor.h"
#include "protoactor/context.h"
#include "protoactor/pid.h"
#include "protoactor/messages.h"
#include <memory>
#include <string>
#include <vector>
#include <chrono>

namespace protoactor {
namespace cluster {

// Forward declarations
class Cluster;

/**
 * @brief PubSub delivery batch request.
 */
struct DeliverBatchRequest : public SystemMessage {
    std::string topic;
    std::vector<std::shared_ptr<MessageEnvelope>> envelopes;
    std::vector<std::shared_ptr<PID>> subscribers;
};

/**
 * @brief PubSub member delivery actor.
 */
class PubSubMemberDeliveryActor : public Actor {
public:
    explicit PubSubMemberDeliveryActor(
        std::chrono::milliseconds subscriber_timeout,
        std::shared_ptr<Cluster> cluster);
    
    void Receive(std::shared_ptr<Context> context) override;

private:
    std::chrono::milliseconds subscriber_timeout_;
    std::shared_ptr<Cluster> cluster_;
    
    void DeliverBatch(
        std::shared_ptr<Context> context,
        const std::string& topic,
        const std::vector<std::shared_ptr<MessageEnvelope>>& envelopes,
        const std::vector<std::shared_ptr<PID>>& subscribers);
};

} // namespace cluster
} // namespace protoactor

#endif // PROTOACTOR_CLUSTER_PUBSUB_DELIVERY_H
