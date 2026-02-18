#ifndef PROTOACTOR_QUEUE_PRIORITY_QUEUE_H
#define PROTOACTOR_QUEUE_PRIORITY_QUEUE_H

#include "../queue.h"
#include <queue>
#include <mutex>
#include <condition_variable>

namespace protoactor {

/**
 * @brief Priority queue item.
 */
struct PriorityQueueItem {
    std::shared_ptr<void> message;
    int priority;
    
    bool operator<(const PriorityQueueItem& other) const {
        return priority < other.priority; // Higher priority first
    }
};

/**
 * @brief Priority queue implementation.
 */
class PriorityQueue : public Queue {
public:
    PriorityQueue();
    
    void Push(std::shared_ptr<void> message) override;
    std::shared_ptr<void> Pop() override;
    bool Empty() const override;
    size_t Size() const override;
    
    // Additional methods
    void Clear();

private:
    std::priority_queue<PriorityQueueItem> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
};

} // namespace protoactor

#endif // PROTOACTOR_QUEUE_PRIORITY_QUEUE_H
