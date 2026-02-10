#ifndef PROTOACTOR_SUPERVISION_H
#define PROTOACTOR_SUPERVISION_H

#include "pid.h"
#include <memory>
#include <chrono>
#include <functional>

namespace protoactor {

// Forward declarations
class ActorSystem;
class Supervisor;
class RestartStatistics;

/**
 * @brief Directive determines how a supervisor should handle a faulting actor.
 */
enum class Directive {
    Resume,    // Resume the actor and continue processing messages
    Restart,   // Discard the actor, replacing it with a new instance
    Stop,      // Stop the actor
    Escalate   // Escalate handling to the parent supervisor
};

/**
 * @brief Decider function type that decides how to handle failures.
 */
using DeciderFunc = std::function<Directive(std::shared_ptr<void> reason)>;

/**
 * @brief SupervisorStrategy interface decides how to handle failing child actors.
 */
class SupervisorStrategy {
public:
    virtual ~SupervisorStrategy() = default;
    
    /**
     * @brief Handle a failure of a child actor.
     * @param actor_system The actor system
     * @param supervisor The supervisor
     * @param child The failing child PID
     * @param rs Restart statistics
     * @param reason Failure reason
     * @param message The message that caused the failure
     */
    virtual void HandleFailure(
        std::shared_ptr<ActorSystem> actor_system,
        std::shared_ptr<Supervisor> supervisor,
        std::shared_ptr<PID> child,
        std::shared_ptr<RestartStatistics> rs,
        std::shared_ptr<void> reason,
        std::shared_ptr<void> message) = 0;
};

/**
 * @brief Supervisor interface for managing child actor lifecycle.
 */
class Supervisor {
public:
    virtual ~Supervisor() = default;
    
    /**
     * @brief Get all child PIDs.
     * @return Vector of child PIDs
     */
    virtual std::vector<std::shared_ptr<PID>> Children() = 0;
    
    /**
     * @brief Escalate a failure to the parent.
     * @param reason Failure reason
     * @param message The message that caused the failure
     */
    virtual void EscalateFailure(std::shared_ptr<void> reason, std::shared_ptr<void> message) = 0;
    
    /**
     * @brief Restart child actors.
     * @param pids PIDs to restart
     */
    virtual void RestartChildren(const std::vector<std::shared_ptr<PID>>& pids) = 0;
    
    /**
     * @brief Stop child actors.
     * @param pids PIDs to stop
     */
    virtual void StopChildren(const std::vector<std::shared_ptr<PID>>& pids) = 0;
    
    /**
     * @brief Resume child actors.
     * @param pids PIDs to resume
     */
    virtual void ResumeChildren(const std::vector<std::shared_ptr<PID>>& pids) = 0;
};

/**
 * @brief Restart statistics for tracking actor restarts.
 */
class RestartStatistics {
public:
    int failure_count;
    std::chrono::system_clock::time_point last_failure_time;
    
    RestartStatistics();
    
    /**
     * @brief Reset the statistics.
     */
    void Reset();
    
    /**
     * @brief Record a failure.
     */
    void Fail();
};

/**
 * @brief Create a OneForOne supervision strategy.
 * @param max_retries Maximum number of retries
 * @param within_duration Time window for retries
 * @param decider Decider function
 * @return Supervisor strategy
 */
std::shared_ptr<SupervisorStrategy> NewOneForOneStrategy(
    int max_retries,
    std::chrono::milliseconds within_duration,
    DeciderFunc decider);

/**
 * @brief Create an AllForOne supervision strategy.
 * @param max_retries Maximum number of retries
 * @param within_duration Time window for retries
 * @param decider Decider function
 * @return Supervisor strategy
 */
std::shared_ptr<SupervisorStrategy> NewAllForOneStrategy(
    int max_retries,
    std::chrono::milliseconds within_duration,
    DeciderFunc decider);

/**
 * @brief Create a Restarting supervision strategy (always restarts).
 * @return Supervisor strategy
 */
std::shared_ptr<SupervisorStrategy> NewRestartingStrategy();

/**
 * @brief Get the default supervision strategy.
 * @return Supervisor strategy
 */
std::shared_ptr<SupervisorStrategy> DefaultSupervisorStrategy();

/**
 * @brief Default decider that always restarts.
 * @param reason Failure reason (unused)
 * @return Restart directive
 */
Directive DefaultDecider(std::shared_ptr<void> reason);

} // namespace protoactor

#endif // PROTOACTOR_SUPERVISION_H
