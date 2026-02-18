#include "protoactor/internal/remote/endpoint_reader.h"
#include "protoactor/remote/remote.h"
#include "protoactor/internal/remote/endpoint_manager.h"
#include "protoactor/internal/remote/serializer.h"
#include "protoactor/internal/remote/messages.h"
#include "protoactor/actor_system.h"
#include "protoactor/internal/process_registry.h"
#include "protoactor/messages.h"
#include <stdexcept>

namespace protoactor {
namespace remote {

EndpointReader::EndpointReader(std::shared_ptr<Remote> remote)
    : remote_(std::move(remote)), suspended_(false) {
}

void EndpointReader::OnMessageBatch(std::shared_ptr<void> batch) {
    if (suspended_.load(std::memory_order_acquire)) {
        return;
    }
    
    // When gRPC is integrated, batch will be protobuf MessageBatch
    // For now, this is a placeholder - full implementation would deserialize and route
    // TODO: Implement message batch handling when gRPC is integrated
}

std::shared_ptr<void> EndpointReader::OnConnectRequest(std::shared_ptr<void> request) {
    // TODO: When gRPC is integrated, request will be protobuf ConnectRequest
    // Handle server connection or client connection
    // Return ConnectResponse
    
    // Placeholder
    return nullptr;
}

void EndpointReader::Suspend(bool suspend) {
    suspended_.store(suspend, std::memory_order_release);
}

void EndpointReader::HandleServerConnection(std::shared_ptr<void> connection) {
    // TODO: When gRPC is integrated:
    // 1. Extract ServerConnection from ConnectRequest
    // 2. Check blocklist
    // 3. Send ConnectResponse (blocked or not)
    // 4. Store connection info
}

void EndpointReader::DeserializeAndDeliver(
    const std::vector<uint8_t>& data,
    const std::string& type_name,
    int32_t serializer_id,
    std::shared_ptr<PID> target,
    std::shared_ptr<PID> sender) {
    
    try {
        // Deserialize message
        auto message = SerializerRegistry::Deserialize(data, type_name, serializer_id);
        
        // Check if it's a Terminated message
        auto terminated = std::dynamic_pointer_cast<Terminated>(
            std::static_pointer_cast<SystemMessage>(message));
        if (terminated) {
            // Handle remote terminate
            auto terminate = std::shared_ptr<RemoteTerminate>(new RemoteTerminate());
            terminate->Watcher = target;
            terminate->Watchee = terminated->who;
            remote_->GetEndpointManager()->RemoteTerminate(terminate->Watcher, terminate->Watchee);
        } else if (IsSystemMessage(message)) {
            // Send system message directly
            auto [process, found] = remote_->GetActorSystem()->GetProcessRegistry()->GetLocal(target->id);
            if (found && process) {
                process->SendSystemMessage(target, message);
            }
        } else {
            // Send user message
            if (sender) {
                auto envelope = std::make_shared<MessageEnvelope>(nullptr, message, sender);
                remote_->GetActorSystem()->GetRoot()->Send(target, envelope);
            } else {
                remote_->GetActorSystem()->GetRoot()->Send(target, message);
            }
        }
    } catch (const std::exception& e) {
        // TODO: Log error
        // Drop message or send to deadletter
    }
}

bool EndpointReader::IsSystemMessage(std::shared_ptr<void> message) {
    // Check if message is a system message type
    auto sys_msg = std::static_pointer_cast<SystemMessage>(message);
    if (!sys_msg) {
        return false;
    }
    
    // Try to cast to various system message types
    return std::dynamic_pointer_cast<Watch>(sys_msg) != nullptr ||
           std::dynamic_pointer_cast<Unwatch>(sys_msg) != nullptr ||
           std::dynamic_pointer_cast<Terminated>(sys_msg) != nullptr ||
           std::dynamic_pointer_cast<Stop>(sys_msg) != nullptr ||
           std::dynamic_pointer_cast<Restart>(sys_msg) != nullptr ||
           std::dynamic_pointer_cast<Started>(sys_msg) != nullptr ||
           std::dynamic_pointer_cast<Stopping>(sys_msg) != nullptr ||
           std::dynamic_pointer_cast<Stopped>(sys_msg) != nullptr ||
           std::dynamic_pointer_cast<Restarting>(sys_msg) != nullptr;
}

} // namespace remote
} // namespace protoactor
