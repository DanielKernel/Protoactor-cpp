#ifndef PROTOACTOR_GUARDIAN_H
#define PROTOACTOR_GUARDIAN_H

#include "supervision.h"
#include "pid.h"
#include <memory>
#include <unordered_map>

namespace protoactor {

// Forward declarations
class ActorSystem;

/**
 * @brief Guardians manages guardian actors for supervision strategies.
 */
class Guardians {
public:
    /**
     * @brief Create new guardians.
     * @param actor_system The actor system
     * @return Guardians instance
     */
    static std::shared_ptr<Guardians> New(std::shared_ptr<ActorSystem> actor_system);
    
    /**
     * @brief Get guardian PID for a strategy.
     * @param strategy Supervisor strategy
     * @return Guardian PID
     */
    std::shared_ptr<PID> GetGuardianPID(std::shared_ptr<SupervisorStrategy> strategy);

private:
    std::shared_ptr<ActorSystem> actor_system_;
    std::unordered_map<SupervisorStrategy*, std::shared_ptr<PID>> guardians_;
    
public:
    explicit Guardians(std::shared_ptr<ActorSystem> actor_system);
};

} // namespace protoactor

#endif // PROTOACTOR_GUARDIAN_H
