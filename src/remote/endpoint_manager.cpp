#include "protoactor/internal/remote/endpoint_manager.h"
#include "protoactor/remote/remote.h"
#include "protoactor/internal/remote/endpoint_writer.h"
#include "protoactor/internal/remote/endpoint_watcher.h"
#include "protoactor/internal/remote/activator_actor.h"
#include "protoactor/internal/remote/messages.h"
#include "protoactor/actor_system.h"
#include "protoactor/props.h"
#include "protoactor/supervision.h"
#include "protoactor/messages.h"
#include "protoactor/eventstream.h"
#include "protoactor/internal/actor/deadletter.h"
#include "protoactor/future.h"
#include <thread>
#include <chrono>

namespace protoactor {
namespace remote {

// Forward declarations for internal actors
class ActivatorActor;
class EndpointSupervisorActor;
class EndpointWatcherActor;

// Endpoint implementation
std::string Endpoint::Address() const {
    if (watcher) {
        return watcher->address;
    }
    return "";
}

// EndpointManager implementation
EndpointManager::EndpointManager(std::shared_ptr<Remote> remote)
    : remote_(std::move(remote)), stopped_(false) {
}

EndpointManager::~EndpointManager() {
    Stop();
}

void EndpointManager::Start() {
    if (stopped_) {
        return;
    }
    
    // Subscribe to endpoint events
    auto event_stream = remote_->GetActorSystem()->GetEventStream();
    endpoint_sub_ = event_stream->SubscribeWithPredicate(
        [this](std::shared_ptr<void> evt) {
            auto terminated = std::dynamic_pointer_cast<EndpointTerminatedEvent>(
                std::static_pointer_cast<SystemMessage>(evt));
            if (terminated) {
                RemoveEndpoint(terminated->Address);
                return;
            }
            
            auto connected = std::dynamic_pointer_cast<EndpointConnectedEvent>(
                std::static_pointer_cast<SystemMessage>(evt));
            if (connected) {
                EnsureConnected(connected->Address);
            }
        },
        [](std::shared_ptr<void> evt) {
            auto terminated = std::dynamic_pointer_cast<EndpointTerminatedEvent>(
                std::static_pointer_cast<SystemMessage>(evt));
            if (terminated) return true;
            
            auto connected = std::dynamic_pointer_cast<EndpointConnectedEvent>(
                std::static_pointer_cast<SystemMessage>(evt));
            return connected != nullptr;
        }
    );
    
    StartActivator();
    StartSupervisor();
    
    // Wait for activator to be ready
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Logger not available in ActorSystem, skip logging for now
    // remote_->GetActorSystem()->Logger()->Info("Started EndpointManager");
}

void EndpointManager::Stop() {
    if (stopped_.exchange(true, std::memory_order_acq_rel)) {
        return; // Already stopped
    }
    
    // Unsubscribe from events
    if (endpoint_sub_) {
        auto sub = std::static_pointer_cast<protoactor::eventstream::Subscription>(endpoint_sub_);
        if (sub) {
            remote_->GetActorSystem()->GetEventStream()->Unsubscribe(sub);
        }
        endpoint_sub_ = nullptr;
    }
    
    StopActivator();
    StopSupervisor();
    
    // Clear connections
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.clear();
    }
    
    // Logger not available in ActorSystem, skip logging for now
    // remote_->GetActorSystem()->Logger()->Info("Stopped EndpointManager");
}

std::shared_ptr<Endpoint> EndpointManager::EnsureConnected(const std::string& address) {
    if (stopped_.load(std::memory_order_acquire)) {
        return nullptr;
    }
    
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    auto it = connections_.find(address);
    if (it != connections_.end()) {
        return it->second;
    }
    
    // Create new endpoint (lazy connection)
    // In full implementation, this would spawn EndpointWriter and EndpointWatcher actors
    // For now, create placeholder
    auto endpoint = std::make_shared<Endpoint>();
    connections_[address] = endpoint;
    
    // TODO: Spawn EndpointWriter and EndpointWatcher via supervisor
    // This would be done by sending address to endpoint_supervisor_
    
    return endpoint;
}

void EndpointManager::RemoveEndpoint(const std::string& address) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.erase(address);
}

void EndpointManager::RemoteDeliver(
    std::shared_ptr<PID> target,
    std::shared_ptr<void> message,
    std::shared_ptr<PID> sender) {
    
    if (stopped_.load(std::memory_order_acquire)) {
        // Send to deadletter
        auto event_stream = remote_->GetActorSystem()->GetEventStream();
        // TODO: Publish DeadLetterEvent when event system supports it
        return;
    }
    
    if (!target || target->address.empty()) {
        return;
    }
    
    auto endpoint = EnsureConnected(target->address);
    if (!endpoint || !endpoint->writer) {
        // Send to deadletter
        auto event_stream = remote_->GetActorSystem()->GetEventStream();
        // TODO: Publish DeadLetterEvent when event system supports it
        return;
    }
    
    // Create remoteDeliver message and send to endpoint writer
    auto deliver = std::shared_ptr<struct RemoteDeliver>(new struct RemoteDeliver());
    deliver->target = target;
    deliver->message = message;
    deliver->sender = sender;
    deliver->serializer_id = -1; // Use default serializer
    
        remote_->GetActorSystem()->GetRoot()->Send(endpoint->writer, deliver);
}

void EndpointManager::RemoteWatch(
    std::shared_ptr<PID> watcher,
    std::shared_ptr<PID> watchee) {
    
    if (stopped_.load(std::memory_order_acquire)) {
        // Send Terminated immediately
        auto terminated = std::make_shared<Terminated>(watchee, protoactor::Terminated::Reason::Stopped);
        auto [process, found] = remote_->GetActorSystem()->GetProcessRegistry()->GetLocal(watcher->id);
        if (found && process) {
            process->SendSystemMessage(watcher, terminated);
        }
        return;
    }
    
    if (!watchee || watchee->address.empty()) {
        return;
    }
    
        auto endpoint = EnsureConnected(watchee->address);
    if (!endpoint || !endpoint->watcher) {
        // Send Terminated with AddressTerminated reason
        auto terminated = std::make_shared<Terminated>(watchee, protoactor::Terminated::Reason::AddressTerminated);
        auto [process, found] = remote_->GetActorSystem()->GetProcessRegistry()->GetLocal(watcher->id);
        if (found && process) {
            process->SendSystemMessage(watcher, terminated);
        }
        return;
    }
    
    // Create remoteWatch message and send to endpoint watcher
    auto watch = std::shared_ptr<struct RemoteWatch>(new struct RemoteWatch());
    watch->Watcher = watcher;
    watch->Watchee = watchee;
        remote_->GetActorSystem()->GetRoot()->Send(endpoint->watcher, watch);
}

void EndpointManager::RemoteUnwatch(
    std::shared_ptr<PID> watcher,
    std::shared_ptr<PID> watchee) {
    
    if (stopped_.load(std::memory_order_acquire)) {
        return;
    }
    
    if (!watchee || watchee->address.empty()) {
        return;
    }
    
        auto endpoint = EnsureConnected(watchee->address);
    if (!endpoint || !endpoint->watcher) {
        return;
    }
    
    // Create remoteUnwatch message and send to endpoint watcher
    auto unwatch = std::shared_ptr<struct RemoteUnwatch>(new struct RemoteUnwatch());
    unwatch->Watcher = watcher;
    unwatch->Watchee = watchee;
        remote_->GetActorSystem()->GetRoot()->Send(endpoint->watcher, unwatch);
}

void EndpointManager::RemoteTerminate(
    std::shared_ptr<PID> watcher,
    std::shared_ptr<PID> watchee) {
    
    if (stopped_.load(std::memory_order_acquire)) {
        return;
    }
    
    if (!watchee || watchee->address.empty()) {
        // Send Terminated with Stopped reason
        auto terminated = std::make_shared<Terminated>(watchee, protoactor::Terminated::Reason::Stopped);
        auto [process, found] = remote_->GetActorSystem()->GetProcessRegistry()->GetLocal(watcher->id);
        if (found && process) {
            process->SendSystemMessage(watcher, terminated);
        }
        return;
    }
    
        auto endpoint = EnsureConnected(watchee->address);
    if (!endpoint || !endpoint->watcher) {
        // Send Terminated with Stopped reason
        auto terminated2 = std::make_shared<Terminated>(watchee, protoactor::Terminated::Reason::Stopped);
        auto [process2, found2] = remote_->GetActorSystem()->GetProcessRegistry()->GetLocal(watcher->id);
        if (found2 && process2) {
            process2->SendSystemMessage(watcher, terminated2);
        }
        return;
    }
    
    // Create remoteTerminate message and send to endpoint watcher
    auto terminate = std::shared_ptr<struct RemoteTerminate>(new struct RemoteTerminate());
    terminate->Watcher = watcher;
    terminate->Watchee = watchee;
        remote_->GetActorSystem()->GetRoot()->Send(endpoint->watcher, terminate);
}

void EndpointManager::StartActivator() {
    auto props = Props::FromProducer([this]() {
        return std::make_shared<ActivatorActor>(remote_);
    });
    auto [pid, err] = remote_->GetActorSystem()->GetRoot()->SpawnNamed(props, "activator");
    if (err) {
        // Logger not available, skip error logging
        // remote_->GetActorSystem()->Logger()->Error("Failed to spawn ActivatorActor", slog::String("error", err.message()));
        return;
    }
    activator_ = pid;
}

void EndpointManager::StopActivator() {
    if (activator_) {
        auto future = remote_->GetActorSystem()->GetRoot()->StopFuture(activator_);
        if (future) {
            future->Wait();
        }
        activator_.reset();
    }
}

void EndpointManager::StartSupervisor() {
    // TODO: Implement EndpointSupervisorActor or use a different approach
    // For now, skip supervisor spawning
    // This will be implemented when EndpointSupervisorActor is defined
}

void EndpointManager::StopSupervisor() {
    if (endpoint_supervisor_) {
        auto future = remote_->GetActorSystem()->GetRoot()->StopFuture(endpoint_supervisor_);
        if (future) {
            future->Wait();
        }
        endpoint_supervisor_.reset();
    }
}

void EndpointManager::HandleEndpointEvent(std::shared_ptr<void> event) {
    // TODO: Handle EndpointTerminatedEvent and EndpointConnectedEvent
    // This would be called from event stream subscription
}

} // namespace remote
} // namespace protoactor
