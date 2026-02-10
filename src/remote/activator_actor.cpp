#include "protoactor/remote/activator_actor.h"
#include "protoactor/remote/remote.h"
#include "protoactor/messages.h"
#include <stdexcept>

namespace protoactor {
namespace remote {

ActivatorActor::ActivatorActor(std::shared_ptr<Remote> remote)
    : remote_(remote) {
}

void ActivatorActor::Receive(std::shared_ptr<Context> context) {
    auto msg = context->Message();
    if (!msg) {
        return;
    }
    
    // Try Started (it's a SystemMessage)
    // Note: We cannot use dynamic_pointer_cast on std::shared_ptr<void>
    // We use static_pointer_cast assuming the message is a SystemMessage if it's a system message
    // In full implementation, we would use type information
    auto sys_msg = std::static_pointer_cast<SystemMessage>(msg);
    if (sys_msg) {
        auto started = std::dynamic_pointer_cast<Started>(sys_msg);
        if (started) {
            // Actor started
            return;
        }
        // Other system messages - ignore
        return;
    }
    
    // Try Ping (user message)
    // Note: We cannot use dynamic_pointer_cast on std::shared_ptr<void>
    // For user messages, we need to use static_cast or type information
    // This is a limitation of the current design - in full implementation,
    // we would use type information or a message registry
    auto ping = std::static_pointer_cast<Ping>(msg);
    if (ping) {
        HandlePing(context);
        return;
    }
    
    // Try ActorPidRequest (user message)
    auto request = std::static_pointer_cast<ActorPidRequest>(msg);
    if (request) {
        HandleActorPidRequest(context, request);
        return;
    }
}

void ActivatorActor::HandlePing(std::shared_ptr<Context> context) {
    context->Respond(std::make_shared<Pong>());
}

void ActivatorActor::HandleActorPidRequest(std::shared_ptr<Context> context, std::shared_ptr<ActorPidRequest> request) {
    if (!remote_) {
        auto response = std::make_shared<ActorPidResponse>(nullptr, 1); // ERROR
        context->Respond(response);
        return;
    }
    
    auto config = remote_->GetConfig();
    if (!config) {
        auto response = std::make_shared<ActorPidResponse>(nullptr, 1); // ERROR
        context->Respond(response);
        return;
    }
    
    // Find props for the kind
    auto it = config->kinds.find(request->kind);
    if (it == config->kinds.end()) {
        auto response = std::make_shared<ActorPidResponse>(nullptr, 1); // ERROR
        context->Respond(response);
        throw std::runtime_error("no Props found for kind " + request->kind);
    }
    
    auto props = it->second;
    std::string name = request->name;
    
    // If name is empty, generate one
    if (name.empty()) {
        name = remote_->GetActorSystem()->GetProcessRegistry()->NextID();
    }
    
    // Spawn the actor
    std::shared_ptr<PID> pid;
    std::error_code err;
    
    if (name.empty()) {
        pid = context->Spawn(props);
    } else {
        std::tie(pid, err) = context->SpawnNamed(props, "Remote$" + name);
    }
    
    if (pid && !err) {
        auto response = std::make_shared<ActorPidResponse>(pid, 0);
        context->Respond(response);
    } else if (err.value() == 1) { // Name exists
        auto response = std::make_shared<ActorPidResponse>(pid, 2); // PROCESSNAMEALREADYEXIST
        context->Respond(response);
    } else {
        auto response = std::make_shared<ActorPidResponse>(nullptr, 1); // ERROR
        context->Respond(response);
        throw std::runtime_error("failed to spawn actor: " + err.message());
    }
}

} // namespace remote
} // namespace protoactor
