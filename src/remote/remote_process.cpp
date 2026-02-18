#include "internal/remote/remote_process.h"
#include "external/remote/remote.h"
#include "internal/remote/endpoint_manager.h"
#include "external/messages.h"
#include <memory>

namespace protoactor {
namespace remote {

RemoteProcess::RemoteProcess(
    std::shared_ptr<PID> pid,
    std::shared_ptr<Remote> remote)
    : pid_(std::move(pid)), remote_(std::move(remote)) {
}

void RemoteProcess::SendUserMessage(std::shared_ptr<PID> pid, std::shared_ptr<void> message) {
    // Extract envelope if present
    auto [header, msg, sender] = UnwrapEnvelope(message);
    remote_->SendMessage(pid, header, msg, sender, -1);
}

void RemoteProcess::SendSystemMessage(std::shared_ptr<PID> pid, std::shared_ptr<void> message) {
    // Handle Watch/Unwatch specially
    auto watch = std::dynamic_pointer_cast<protoactor::Watch>(
        std::static_pointer_cast<protoactor::SystemMessage>(message));
    if (watch && watch->watcher) {
        remote_->GetEndpointManager()->RemoteWatch(watch->watcher, pid);
        return;
    }
    
    auto unwatch = std::dynamic_pointer_cast<protoactor::Unwatch>(
        std::static_pointer_cast<protoactor::SystemMessage>(message));
    if (unwatch && unwatch->watcher) {
        remote_->GetEndpointManager()->RemoteUnwatch(unwatch->watcher, pid);
        return;
    }
    
    remote_->SendMessage(pid, nullptr, message, nullptr, -1);
}

void RemoteProcess::Stop(std::shared_ptr<PID> pid) {
    auto stop = std::make_shared<protoactor::Stop>();
    SendSystemMessage(pid, stop);
}

} // namespace remote
} // namespace protoactor
