#ifndef PROTOACTOR_FUTURE_H
#define PROTOACTOR_FUTURE_H

#include "pid.h"
#include <memory>
#include <chrono>
#include <system_error>
#include <vector>
#include <functional>
#include <mutex>
#include <condition_variable>

namespace protoactor {

// Forward declarations
class ActorSystem;

/**
 * @brief Future defines the public interface for future responses.
 */
class Future {
public:
    virtual ~Future() = default;
    
    /**
     * @brief Get the PID to the backing actor for the Future result.
     * @return PID
     */
    virtual std::shared_ptr<PID> GetPID() = 0;
    
    /**
     * @brief Pipe the result or error to the specified PIDs.
     * @param pids Target PIDs
     */
    virtual void PipeTo(const std::vector<std::shared_ptr<PID>>& pids) = 0;
    
    /**
     * @brief Wait for the future to resolve and return the result or error.
     * @return Pair of result and error code
     */
    virtual std::pair<std::shared_ptr<void>, std::error_code> Result() = 0;
    
    /**
     * @brief Wait blocks until the future resolves and returns the error, if any.
     * @return Error code
     */
    virtual std::error_code Wait() = 0;
    
    /**
     * @brief Continue with a continuation function.
     * @param continuation Function to call when future completes
     */
    virtual void ContinueWith(std::function<void(std::shared_ptr<void>, std::error_code)> continuation) = 0;
};

/**
 * @brief Create a new Future with a timeout.
 * @param actor_system The actor system
 * @param timeout Timeout duration
 * @return Future instance
 */
std::shared_ptr<Future> NewFuture(std::shared_ptr<ActorSystem> actor_system, std::chrono::milliseconds timeout);

} // namespace protoactor

#endif // PROTOACTOR_FUTURE_H
