#include "internal/queue.h"
#include <queue>
#include <mutex>

namespace protoactor {

// Simple unbounded queue implementation
class UnboundedQueue : public Queue {
public:
    void Push(std::shared_ptr<void> item) override {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(item);
    }
    
    std::shared_ptr<void> Pop() override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return nullptr;
        }
        auto item = queue_.front();
        queue_.pop();
        return item;
    }
    
    bool Empty() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }
    
    size_t Size() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    mutable std::mutex mutex_;
    std::queue<std::shared_ptr<void>> queue_;
};

// MPSC (Multi-Producer Single-Consumer) queue
// Simplified implementation using mutex
class MPSCQueueImpl : public MPSCQueue {
public:
    void Push(std::shared_ptr<void> item) override {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(item);
    }
    
    std::shared_ptr<void> Pop() override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return nullptr;
        }
        auto item = queue_.front();
        queue_.pop();
        return item;
    }

private:
    std::mutex mutex_;
    std::queue<std::shared_ptr<void>> queue_;
};

std::shared_ptr<Queue> NewUnboundedQueue() {
    return std::make_shared<UnboundedQueue>();
}

std::shared_ptr<MPSCQueue> NewMPSCQueue() {
    return std::make_shared<MPSCQueueImpl>();
}

} // namespace protoactor
