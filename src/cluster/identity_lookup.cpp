#include "internal/cluster/identity_lookup.h"
#include "external/cluster/cluster.h"
#include "internal/cluster/member_list.h"
#include "internal/cluster/member.h"
#include "internal/cluster/pid_cache.h"
#include "external/remote/remote.h"
#include "external/future.h"
#include <stdexcept>

namespace protoactor {
namespace cluster {

void DefaultIdentityLookup::Setup(
    std::shared_ptr<Cluster> cluster,
    const std::vector<std::string>& kinds,
    bool is_client) {
    cluster_ = cluster;
    kinds_ = kinds;
    is_client_ = is_client;
}

std::pair<std::shared_ptr<PID>, std::error_code> DefaultIdentityLookup::Get(
    std::shared_ptr<Cluster> cluster,
    const std::string& identity,
    const std::string& kind,
    std::chrono::milliseconds timeout) {
    
    if (!cluster) {
        return {nullptr, std::make_error_code(std::errc::invalid_argument)};
    }
    
    // Check cache first
    auto cache = cluster->GetPidCache();
    if (cache) {
        auto cached_pid = cache->Get(identity);
        if (cached_pid) {
            return {cached_pid, std::error_code()};
        }
    }
    
    // Find member that owns this identity (using consistent hash)
    auto member_list = cluster->GetMemberList();
    if (!member_list) {
        return {nullptr, std::make_error_code(std::errc::no_such_device)};
    }
    
    auto members = member_list->GetMembers();
    if (members.empty()) {
        return {nullptr, std::make_error_code(std::errc::network_unreachable)};
    }
    
    // Simple hash-based selection (in full implementation, would use consistent hash)
    std::hash<std::string> hasher;
    size_t hash = hasher(identity + kind);
    auto member = members[hash % members.size()];
    
    if (!member) {
        return {nullptr, std::make_error_code(std::errc::no_such_device)};
    }
    
    // Spawn remote actor
    auto remote = cluster->GetRemote();
    if (!remote) {
        return {nullptr, std::make_error_code(std::errc::no_such_device)};
    }
    
    auto [pid, err] = remote->SpawnNamed(member->Address(), kind, identity, timeout);
    if (err || !pid) {
        return {nullptr, err};
    }
    
    // Cache the PID
    if (cache) {
        cache->Add(identity, pid);
    }
    
    return {pid, std::error_code()};
}

void DefaultIdentityLookup::RemoveCache(const std::string& identity, const std::string& kind) {
    if (cluster_) {
        auto cache = cluster_->GetPidCache();
        if (cache) {
            cache->Remove(identity);
        }
    }
}

void DefaultIdentityLookup::Shutdown() {
    cluster_.reset();
    kinds_.clear();
}

} // namespace cluster
} // namespace protoactor
