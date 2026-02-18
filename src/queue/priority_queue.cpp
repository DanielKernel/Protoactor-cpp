#include "protoactor/priority_queue.h"
#include <algorithm>

namespace protoactor {

PriorityQueue::PriorityQueue() {
}

void PriorityQueue::Push(std::shared_ptr<void> message) {
    std::lock_guard<std::mutex> lock(mutex_);
    PriorityQueueItem item;
    item.message = message;
    item.priority = 0; // Default priority, would be extracted from message in full implementation
    queue_.push(item);
    cv_.notify_one();
}

std::shared_ptr<void> PriorityQueue::Pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !queue_.empty(); });
    
    auto item = queue_.top();
    queue_.pop();
    return item.message;
}

size_t PriorityQueue::Size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

bool PriorityQueue::Empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

void PriorityQueue::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!queue_.empty()) {
        queue_.pop();
    }
}

} // namespace protoactor
