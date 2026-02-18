#include "protoactor/internal/cluster/pubsub_delivery.h"
#include "protoactor/cluster/cluster.h"
#include "protoactor/future.h"
#include "protoactor/messages.h"
#include <algorithm>

namespace protoactor {
namespace cluster {

PubSubMemberDeliveryActor::PubSubMemberDeliveryActor(
    std::chrono::milliseconds subscriber_timeout,
    std::shared_ptr<Cluster> cluster)
    : subscriber_timeout_(subscriber_timeout), cluster_(cluster) {
}

void PubSubMemberDeliveryActor::Receive(std::shared_ptr<Context> context) {
    auto msg = context->Message();
    if (!msg) {
        return;
    }
    
    // Try DeliverBatchRequest
    auto batch = std::dynamic_pointer_cast<DeliverBatchRequest>(
        std::static_pointer_cast<SystemMessage>(msg));
    if (batch) {
        DeliverBatch(
            context,
            batch->topic,
            batch->envelopes,
            batch->subscribers);
    }
}

void PubSubMemberDeliveryActor::DeliverBatch(
    std::shared_ptr<Context> context,
    const std::string& topic,
    const std::vector<std::shared_ptr<MessageEnvelope>>& envelopes,
    const std::vector<std::shared_ptr<PID>>& subscribers) {
    
    // Deliver to each subscriber
    for (const auto& subscriber : subscribers) {
        if (!subscriber) {
            continue;
        }
        
        // Send each envelope to subscriber
        for (const auto& envelope : envelopes) {
            if (envelope && envelope->message) {
                // Use RequestFuture for delivery with timeout
                auto future = context->RequestFuture(
                    subscriber,
                    envelope->message,
                    subscriber_timeout_);
                
                // Fire and forget - errors are handled by the delivery actor
                // In full implementation, would track delivery status
            }
        }
    }
}

} // namespace cluster
} // namespace protoactor
