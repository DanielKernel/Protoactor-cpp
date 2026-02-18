#ifndef PROTOACTOR_SCHEDULER_TIMER_H
#define PROTOACTOR_SCHEDULER_TIMER_H

#include "protoactor/context.h"
#include "protoactor/pid.h"
#include <memory>
#include <chrono>
#include <functional>

namespace protoactor {
namespace scheduler {

// Forward declaration
using SenderContext = std::shared_ptr<Context>;

/**
 * @brief Cancel function type for canceling scheduled tasks.
 */
using CancelFunc = std::function<void()>;

/**
 * @brief Timer scheduler for sending messages in the future and at regular intervals.
 */
class TimerScheduler {
public:
    /**
     * @brief Create a new timer scheduler.
     * @param sender Sender context
     * @return Timer scheduler instance
     */
    static std::shared_ptr<TimerScheduler> New(std::shared_ptr<Context> sender);
    
    /**
     * @brief Send a message once after a delay.
     * @param delay Delay duration
     * @param pid Target PID
     * @param message Message to send
     * @return Cancel function
     */
    CancelFunc SendOnce(
        std::chrono::milliseconds delay,
        std::shared_ptr<PID> pid,
        std::shared_ptr<void> message);
    
    /**
     * @brief Send a message repeatedly at intervals.
     * @param initial Initial delay
     * @param interval Interval duration
     * @param pid Target PID
     * @param message Message to send
     * @return Cancel function
     */
    CancelFunc SendRepeatedly(
        std::chrono::milliseconds initial,
        std::chrono::milliseconds interval,
        std::shared_ptr<PID> pid,
        std::shared_ptr<void> message);
    
    /**
     * @brief Request a message once after a delay.
     * @param delay Delay duration
     * @param pid Target PID
     * @param message Message to send
     * @return Cancel function
     */
    CancelFunc RequestOnce(
        std::chrono::milliseconds delay,
        std::shared_ptr<PID> pid,
        std::shared_ptr<void> message);
    
    /**
     * @brief Request a message repeatedly at intervals.
     * @param delay Initial delay
     * @param interval Interval duration
     * @param pid Target PID
     * @param message Message to send
     * @return Cancel function
     */
    CancelFunc RequestRepeatedly(
        std::chrono::milliseconds delay,
        std::chrono::milliseconds interval,
        std::shared_ptr<PID> pid,
        std::shared_ptr<void> message);

private:
    std::shared_ptr<Context> ctx_;
    
public:
    TimerScheduler(std::shared_ptr<Context> sender);
};

} // namespace scheduler
} // namespace protoactor

#endif // PROTOACTOR_SCHEDULER_TIMER_H
