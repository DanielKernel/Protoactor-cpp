#include "external/pid.h"
#include "external/actor_system.h"
#include "internal/process_registry.h"
#include "internal/process.h"
#include <atomic>

namespace protoactor {

PID::PID(const std::string& addr, const std::string& identifier)
    : address(addr), id(identifier), request_id(0), process_ptr_(nullptr) {
}

bool PID::Equal(const std::shared_ptr<PID>& other) const {
    if (!other) {
        return false;
    }
    return id == other->id && address == other->address && request_id == other->request_id;
}

std::shared_ptr<Process> PID::Ref(std::shared_ptr<ActorSystem> actor_system) {
    // Always get from registry to ensure we have a proper shared_ptr
    // Caching raw pointers is unsafe because the process might be destroyed
    auto registry = actor_system->GetProcessRegistry();
    auto [process, exists] = registry->Get(shared_from_this());
    
    if (exists && process) {
        // Cache the raw pointer for quick lookup (but don't use it for shared_ptr creation)
        process_ptr_.store(process.get(), std::memory_order_release);
    }
    
    return process;
}

void PID::SendUserMessage(std::shared_ptr<ActorSystem> actor_system, std::shared_ptr<void> message) {
    auto process = Ref(actor_system);
    if (process) {
        process->SendUserMessage(shared_from_this(), message);
    }
}

void PID::SendSystemMessage(std::shared_ptr<ActorSystem> actor_system, std::shared_ptr<void> message) {
    auto process = Ref(actor_system);
    if (process) {
        process->SendSystemMessage(shared_from_this(), message);
    }
}

void PID::ClearCache() {
    process_ptr_.store(nullptr, std::memory_order_release);
}


} // namespace protoactor
