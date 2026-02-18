#include "external/eventstream.h"
#include <algorithm>

namespace protoactor {
namespace eventstream {

Subscription::Subscription(Handler handler)
    : id_(0), handler_(handler), active_(1) {
}

bool Subscription::Activate() {
    uint32_t expected = 0;
    return active_.compare_exchange_strong(expected, 1);
}

bool Subscription::Deactivate() {
    uint32_t expected = 1;
    return active_.compare_exchange_strong(expected, 0);
}

bool Subscription::IsActive() const {
    return active_.load() == 1;
}

EventStream::EventStream()
    : counter_(0) {
}

std::shared_ptr<EventStream> EventStream::New() {
    return std::make_shared<EventStream>();
}

std::shared_ptr<Subscription> EventStream::Subscribe(Handler handler) {
    auto sub = std::make_shared<Subscription>(handler);
    
    std::lock_guard<std::mutex> lock(mutex_);
    sub->id_ = counter_.load();
    counter_.fetch_add(1);
    subscriptions_.push_back(sub);
    
    return sub;
}

std::shared_ptr<Subscription> EventStream::SubscribeWithPredicate(Handler handler, Predicate predicate) {
    auto sub = Subscribe(handler);
    sub->predicate_ = predicate;
    return sub;
}

void EventStream::Unsubscribe(std::shared_ptr<Subscription> subscription) {
    if (!subscription) {
        return;
    }
    
    if (subscription->IsActive()) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (subscription->Deactivate()) {
            auto it = std::find_if(subscriptions_.begin(), subscriptions_.end(),
                [subscription](const std::shared_ptr<Subscription>& sub) {
                    return sub->id_ == subscription->id_;
                });
            
            if (it != subscriptions_.end()) {
                subscriptions_.erase(it);
                counter_.fetch_sub(1);
            }
        }
    }
}

void EventStream::Publish(std::shared_ptr<void> evt) {
    std::vector<std::shared_ptr<Subscription>> subs;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        subs.reserve(subscriptions_.size());
        for (auto& sub : subscriptions_) {
            if (sub->IsActive()) {
                subs.push_back(sub);
            }
        }
    }
    
    for (auto& sub : subs) {
        // Check predicate if present
        if (sub->predicate_ && !sub->predicate_(evt)) {
            continue;
        }
        
        // Execute handler
        sub->handler_(evt);
    }
}

int32_t EventStream::Length() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return counter_.load();
}

} // namespace eventstream
} // namespace protoactor
