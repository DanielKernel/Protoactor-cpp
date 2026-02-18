#ifndef PROTOACTOR_EVENTSTREAM_EVENTSTREAM_H
#define PROTOACTOR_EVENTSTREAM_EVENTSTREAM_H

#include <memory>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>

namespace protoactor {
namespace eventstream {

// Forward declarations
class Subscription;

/**
 * @brief Handler function type for event callbacks.
 */
using Handler = std::function<void(std::shared_ptr<void>)>;

/**
 * @brief Predicate function type for filtering events.
 */
using Predicate = std::function<bool(std::shared_ptr<void>)>;

/**
 * @brief EventStream is a threadsafe publish-subscribe message bus.
 */
class EventStream {
public:
    /**
     * @brief Create a new event stream.
     * @return Event stream instance
     */
    static std::shared_ptr<EventStream> New();
    
    /**
     * @brief Subscribe to events with a handler.
     * @param handler Event handler
     * @return Subscription
     */
    std::shared_ptr<Subscription> Subscribe(Handler handler);
    
    /**
     * @brief Subscribe with a predicate filter.
     * @param handler Event handler
     * @param predicate Filter predicate
     * @return Subscription
     */
    std::shared_ptr<Subscription> SubscribeWithPredicate(Handler handler, Predicate predicate);
    
    /**
     * @brief Unsubscribe from events.
     * @param subscription The subscription to remove
     */
    void Unsubscribe(std::shared_ptr<Subscription> subscription);
    
    /**
     * @brief Publish an event to all subscribers.
     * @param evt The event
     */
    void Publish(std::shared_ptr<void> evt);
    
    /**
     * @brief Get the number of subscribers.
     * @return Subscriber count
     */
    int32_t Length() const;

private:
    mutable std::mutex mutex_;
    std::vector<std::shared_ptr<Subscription>> subscriptions_;
    std::atomic<int32_t> counter_;
    
public:
    EventStream();
};

/**
 * @brief Subscription represents a registered handler and its state.
 */
class Subscription {
public:
    /**
     * @brief Activate the subscription.
     * @return true if state changed
     */
    bool Activate();
    
    /**
     * @brief Deactivate the subscription.
     * @return true if state changed
     */
    bool Deactivate();
    
    /**
     * @brief Check if the subscription is active.
     * @return true if active
     */
    bool IsActive() const;
    
    friend class EventStream;

private:
    int32_t id_;
    Handler handler_;
    Predicate predicate_;
    mutable std::atomic<uint32_t> active_;
    
public:
    explicit Subscription(Handler handler);
};

} // namespace eventstream
} // namespace protoactor

#endif // PROTOACTOR_EVENTSTREAM_EVENTSTREAM_H
