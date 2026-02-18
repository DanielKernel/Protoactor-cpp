#ifndef PROTOACTOR_ACTOR_H
#define PROTOACTOR_ACTOR_H

#include <memory>
#include <string>
#include <cstdint>
#include <functional>

namespace protoactor {

// Forward declarations
class Context;
class ActorSystem;
class PID;

/**
 * @brief Actor interface that defines the Receive method.
 * 
 * Receive is sent messages to be processed from the mailbox associated 
 * with the instance of the actor.
 */
class Actor {
public:
    virtual ~Actor() = default;
    
    /**
     * @brief Process messages from the mailbox.
     * @param context The actor context containing message and system information.
     */
    virtual void Receive(std::shared_ptr<Context> context) = 0;
};

/**
 * @brief Producer function type that creates a new actor instance.
 */
using Producer = std::function<std::shared_ptr<Actor>()>;

/**
 * @brief Producer function type that creates a new actor instance with access to the actor system.
 */
using ProducerWithActorSystem = std::function<std::shared_ptr<Actor>(std::shared_ptr<ActorSystem>)>;

} // namespace protoactor

#endif // PROTOACTOR_ACTOR_H
