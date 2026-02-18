#include "internal/process_registry.h"
#include "external/actor_system.h"
#include "internal/process.h"
#include "external/pid.h"
#include "internal/actor/deadletter.h"
#include "internal/actor/new_pid.h"
#include <cstring>
#include <sstream>
#include <iomanip>

namespace protoactor {

constexpr const char* LOCAL_ADDRESS = "nonhost";
constexpr const char* DIGITS = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ~+";

ProcessRegistry::ProcessRegistry(std::shared_ptr<ActorSystem> actor_system)
    : sequence_id_(0), actor_system_(actor_system), address_(LOCAL_ADDRESS) {
    for (auto& shard : local_pids_) {
        shard.clear();
    }
}

std::shared_ptr<ProcessRegistry> ProcessRegistry::New(std::shared_ptr<ActorSystem> actor_system) {
    return std::make_shared<ProcessRegistry>(actor_system);
}

int ProcessRegistry::GetBucket(const std::string& key) const {
    // Simple hash function
    size_t hash = 0;
    for (char c : key) {
        hash = hash * 31 + c;
    }
    return static_cast<int>(hash % NUM_SHARDS);
}

std::string ProcessRegistry::Uint64ToID(uint64_t u) {
    std::string result = "$";
    while (u >= 64) {
        result = DIGITS[u & 0x3f] + result;
        u >>= 6;
    }
    result = DIGITS[u] + result;
    return result;
}

std::string ProcessRegistry::NextID() {
    uint64_t counter = ++sequence_id_;
    return Uint64ToID(counter);
}

std::pair<std::shared_ptr<PID>, bool> ProcessRegistry::Add(
    std::shared_ptr<Process> process,
    const std::string& id) {
    int bucket = GetBucket(id);
    std::lock_guard<std::mutex> lock(shard_mutexes_[bucket]);
    
    auto& shard = local_pids_[bucket];
    if (shard.find(id) != shard.end()) {
        // Already exists
        auto pid = NewPID(address_, id);
        return {pid, false};
    }
    
    shard[id] = process;
    auto pid = NewPID(address_, id);
    return {pid, true};
}

void ProcessRegistry::Remove(std::shared_ptr<PID> pid) {
    if (!pid) {
        return;
    }
    
    int bucket = GetBucket(pid->id);
    std::lock_guard<std::mutex> lock(shard_mutexes_[bucket]);
    
    auto& shard = local_pids_[bucket];
    shard.erase(pid->id);
}

std::pair<std::shared_ptr<Process>, bool> ProcessRegistry::Get(std::shared_ptr<PID> pid) {
    if (!pid) {
        return {actor_system_->GetDeadLetter(), false};
    }
    
    if (pid->address != LOCAL_ADDRESS && pid->address != address_) {
        // Try remote handlers
        for (auto& handler : remote_handlers_) {
            auto [process, ok] = handler(pid);
            if (ok) {
                return {process, true};
            }
        }
        return {actor_system_->GetDeadLetter(), false};
    }
    
    int bucket = GetBucket(pid->id);
    std::lock_guard<std::mutex> lock(shard_mutexes_[bucket]);
    
    auto& shard = local_pids_[bucket];
    auto it = shard.find(pid->id);
    if (it == shard.end()) {
        return {actor_system_->GetDeadLetter(), false};
    }
    
    return {it->second, true};
}

std::pair<std::shared_ptr<Process>, bool> ProcessRegistry::GetLocal(const std::string& id) {
    int bucket = GetBucket(id);
    std::lock_guard<std::mutex> lock(shard_mutexes_[bucket]);

    auto& shard = local_pids_[bucket];
    auto it = shard.find(id);
    if (it == shard.end()) {
        return {actor_system_->GetDeadLetter(), false};
    }

    return {it->second, true};
}

void ProcessRegistry::Clear() {
    for (int i = 0; i < NUM_SHARDS; ++i) {
        std::lock_guard<std::mutex> lock(shard_mutexes_[i]);
        local_pids_[i].clear();
    }
}

void ProcessRegistry::RegisterAddressResolver(AddressResolver resolver) {
    remote_handlers_.push_back(resolver);
}

std::string ProcessRegistry::Address() const {
    return address_;
}

} // namespace protoactor
