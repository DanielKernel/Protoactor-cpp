#ifndef PROTOACTOR_DISPATCHER_H
#define PROTOACTOR_DISPATCHER_H

#include <functional>
#include <memory>

namespace protoactor {

/**
 * @brief Dispatcher schedules work for actors and reports processing throughput.
 */
class Dispatcher {
public:
    virtual ~Dispatcher() = default;
    
    /**
     * @brief Schedule a function for execution.
     * @param fn The function to execute
     */
    virtual void Schedule(std::function<void()> fn) = 0;
    
    /**
     * @brief Get the number of messages processed per scheduling pass.
     * @return Throughput value
     */
    virtual int Throughput() const = 0;
};

/**
 * @brief Create a default dispatcher that schedules work on separate threads.
 * @param throughput Messages per pass
 * @return Dispatcher instance
 */
std::shared_ptr<Dispatcher> NewDefaultDispatcher(int throughput);

/**
 * @brief Create a synchronized dispatcher that executes work sequentially.
 * @param throughput Messages per pass
 * @return Dispatcher instance
 */
std::shared_ptr<Dispatcher> NewSynchronizedDispatcher(int throughput);

} // namespace protoactor

#endif // PROTOACTOR_DISPATCHER_H
