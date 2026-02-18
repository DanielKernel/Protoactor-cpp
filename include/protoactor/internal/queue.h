#ifndef PROTOACTOR_QUEUE_H
#define PROTOACTOR_QUEUE_H

#include <memory>
#include <cstddef>

namespace protoactor {

/**
 * @brief Queue interface for message queues.
 */
class Queue {
public:
    virtual ~Queue() = default;
    
    /**
     * @brief Push an item to the queue.
     * @param item Item to push
     */
    virtual void Push(std::shared_ptr<void> item) = 0;
    
    /**
     * @brief Pop an item from the queue.
     * @return Item or nullptr if empty
     */
    virtual std::shared_ptr<void> Pop() = 0;
    
    /**
     * @brief Check if queue is empty.
     * @return true if empty
     */
    virtual bool Empty() const = 0;
    
    /**
     * @brief Get queue size.
     * @return Size
     */
    virtual size_t Size() const = 0;
};

/**
 * @brief MPSC queue (Multi-Producer Single-Consumer).
 */
class MPSCQueue {
public:
    virtual ~MPSCQueue() = default;
    
    virtual void Push(std::shared_ptr<void> item) = 0;
    virtual std::shared_ptr<void> Pop() = 0;
};

// Forward declaration for implementation
class MPSCQueueImpl;

/**
 * @brief Create a new unbounded queue.
 * @return Queue instance
 */
std::shared_ptr<Queue> NewUnboundedQueue();

/**
 * @brief Create a new MPSC queue.
 * @return MPSC queue instance
 */
std::shared_ptr<MPSCQueue> NewMPSCQueue();

} // namespace protoactor

#endif // PROTOACTOR_QUEUE_H
