#include "protoactor/internal/cluster/pid_cache.h"
#include "protoactor/internal/cluster/member.h"
#include <algorithm>

namespace protoactor {
namespace cluster {

PidCache::PidCache() {
}

std::shared_ptr<PID> PidCache::Get(const std::string& identity) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cache_.find(identity);
    if (it != cache_.end()) {
        return it->second;
    }
    return nullptr;
}

void PidCache::Add(const std::string& identity, std::shared_ptr<PID> pid) {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_[identity] = pid;
}

void PidCache::Remove(const std::string& identity) {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.erase(identity);
}

void PidCache::RemoveByMember(std::shared_ptr<Member> member) {
    if (!member) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    std::string member_address = member->Address();
    
    // Remove all PIDs that belong to this member
    auto it = cache_.begin();
    while (it != cache_.end()) {
        if (it->second && it->second->address == member_address) {
            it = cache_.erase(it);
        } else {
            ++it;
        }
    }
}

void PidCache::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.clear();
}

} // namespace cluster
} // namespace protoactor
