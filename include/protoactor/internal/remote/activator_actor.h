#ifndef PROTOACTOR_REMOTE_ACTIVATOR_ACTOR_H
#define PROTOACTOR_REMOTE_ACTIVATOR_ACTOR_H

#include "protoactor/actor.h"
#include "protoactor/context.h"
#include "protoactor/pid.h"
#include "protoactor/props.h"
#include "protoactor/remote/remote.h"
#include "protoactor/internal/remote/messages.h" // For Ping, Pong, ActorPidRequest/Response
#include <memory>
#include <string>

namespace protoactor {
namespace remote {

/**
 * @brief ActivatorActor handles remote actor activation requests.
 */
class ActivatorActor : public Actor {
public:
    explicit ActivatorActor(std::shared_ptr<Remote> remote);
    
    void Receive(std::shared_ptr<Context> context) override;

private:
    std::shared_ptr<Remote> remote_;
    
    void HandleActorPidRequest(std::shared_ptr<Context> context, std::shared_ptr<protoactor::remote::ActorPidRequest> request);
    void HandlePing(std::shared_ptr<Context> context);
};

// ActorPidRequest, ActorPidResponse, Ping, Pong are defined in messages.h

} // namespace remote
} // namespace protoactor

#endif // PROTOACTOR_REMOTE_ACTIVATOR_ACTOR_H
