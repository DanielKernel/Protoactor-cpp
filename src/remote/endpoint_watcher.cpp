#include "internal/remote/endpoint_watcher.h"
#include "external/remote/remote.h"
#include "internal/remote/messages.h"
#include "external/messages.h"
#include "internal/process_registry.h"
#include <algorithm>

namespace protoactor {
namespace remote {

EndpointWatcher::EndpointWatcher(std::shared_ptr<Remote> remote, const std::string& address)
    : remote_(remote), address_(address) {
}

void EndpointWatcher::Receive(std::shared_ptr<Context> context) {
    auto msg = context->Message();
    if (!msg) {
        return;
    }
    
    // Try Started
    auto started = std::dynamic_pointer_cast<Started>(
        std::static_pointer_cast<SystemMessage>(msg));
    if (started) {
        // Initialize
        return;
    }
    
    // Try RemoteWatch
    auto watch = std::dynamic_pointer_cast<RemoteWatch>(
        std::static_pointer_cast<SystemMessage>(msg));
    if (watch) {
        HandleRemoteWatch(context, watch);
        return;
    }
    
    // Try RemoteUnwatch
    auto unwatch = std::dynamic_pointer_cast<RemoteUnwatch>(
        std::static_pointer_cast<SystemMessage>(msg));
    if (unwatch) {
        HandleRemoteUnwatch(context, unwatch);
        return;
    }
    
    // Try RemoteTerminate
    auto terminate = std::dynamic_pointer_cast<RemoteTerminate>(
        std::static_pointer_cast<SystemMessage>(msg));
    if (terminate) {
        HandleRemoteTerminate(context, terminate);
        return;
    }
    
    // Try EndpointTerminatedEvent
    auto terminated = std::dynamic_pointer_cast<EndpointTerminatedEvent>(
        std::static_pointer_cast<SystemMessage>(msg));
    if (terminated) {
        HandleEndpointTerminated(context);
        return;
    }
    
    // Try EndpointConnectedEvent
    auto connected = std::dynamic_pointer_cast<EndpointConnectedEvent>(
        std::static_pointer_cast<SystemMessage>(msg));
    if (connected) {
        HandleEndpointConnected(context);
        return;
    }
    
    // Ignore other system messages
    // Note: We cannot use dynamic_pointer_cast on std::shared_ptr<void>
    // We assume that if msg is not null and not one of the above types, it might be a system message
    // In full implementation, we would use type information
    auto sys_msg = std::static_pointer_cast<SystemMessage>(msg);
    if (sys_msg) {
        return;
    }
}

void EndpointWatcher::HandleRemoteWatch(std::shared_ptr<Context> context, std::shared_ptr<RemoteWatch> watch) {
    if (!watch || !watch->Watcher || !watch->Watchee) {
        return;
    }
    
    // Add watchee to watcher's map
    std::string watcher_id = watch->Watcher->id;
    auto& watchees = watched_[watcher_id];
    
    // Check if already watching
    auto it = std::find_if(watchees.begin(), watchees.end(),
        [watch](const std::shared_ptr<PID>& p) {
            return p && watch->Watchee && p->id == watch->Watchee->id;
        });
    
    if (it == watchees.end()) {
        watchees.push_back(watch->Watchee);
    }
    
    // Forward Watch to remote PID
    auto watch_msg = std::make_shared<Watch>(watch->Watcher);
    remote_->SendMessage(watch->Watchee, nullptr, watch_msg, nullptr, -1);
}

void EndpointWatcher::HandleRemoteUnwatch(std::shared_ptr<Context> context, std::shared_ptr<RemoteUnwatch> unwatch) {
    if (!unwatch || !unwatch->Watcher || !unwatch->Watchee) {
        return;
    }
    
    // Remove watchee from watcher's map
    std::string watcher_id = unwatch->Watcher->id;
    auto it = watched_.find(watcher_id);
    if (it != watched_.end()) {
        auto& watchees = it->second;
        watchees.erase(
            std::remove_if(watchees.begin(), watchees.end(),
                [unwatch](const std::shared_ptr<PID>& p) {
                    return p && unwatch->Watchee && p->id == unwatch->Watchee->id;
                }),
            watchees.end());
        
        if (watchees.empty()) {
            watched_.erase(it);
        }
    }
    
    // Forward Unwatch to remote PID
    auto unwatch_msg = std::make_shared<Unwatch>(unwatch->Watcher);
    remote_->SendMessage(unwatch->Watchee, nullptr, unwatch_msg, nullptr, -1);
}

void EndpointWatcher::HandleRemoteTerminate(std::shared_ptr<Context> context, std::shared_ptr<RemoteTerminate> terminate) {
    if (!terminate || !terminate->Watcher || !terminate->Watchee) {
        return;
    }
    
    // Remove watchee from watcher's map
    std::string watcher_id = terminate->Watcher->id;
    auto it = watched_.find(watcher_id);
    if (it != watched_.end()) {
        auto& watchees = it->second;
        watchees.erase(
            std::remove_if(watchees.begin(), watchees.end(),
                [terminate](const std::shared_ptr<PID>& p) {
                    return p && terminate->Watchee && p->id == terminate->Watchee->id;
                }),
            watchees.end());
        
        if (watchees.empty()) {
            watched_.erase(it);
        }
    }
    
    // Send Terminated message to watcher
    auto terminated = std::make_shared<Terminated>(terminate->Watchee, protoactor::Terminated::Reason::Stopped);
    auto [process, found] = remote_->GetActorSystem()->GetProcessRegistry()->GetLocal(terminate->Watcher->id);
    if (found && process) {
        process->SendSystemMessage(terminate->Watcher, terminated);
    }
}

void EndpointWatcher::HandleEndpointTerminated(std::shared_ptr<Context> context) {
    // Notify all watchers about terminated actors
    auto registry = remote_->GetActorSystem()->GetProcessRegistry();
    
    for (const auto& [watcher_id, watchees] : watched_) {
        auto [process, found] = registry->GetLocal(watcher_id);
        if (found && process) {
            for (const auto& watchee : watchees) {
                auto terminated = std::make_shared<Terminated>(
                    watchee, protoactor::Terminated::Reason::AddressTerminated);
                auto watcher_pid = remote_->GetActorSystem()->NewLocalPID(watcher_id);
                process->SendSystemMessage(watcher_pid, terminated);
            }
        }
    }
    
    // Clear watched map
    watched_.clear();
    
    // Stop self
    context->Stop(context->Self());
}

void EndpointWatcher::HandleEndpointConnected(std::shared_ptr<Context> context) {
    // Endpoint reconnected, already in connected state
}

} // namespace remote
} // namespace protoactor
