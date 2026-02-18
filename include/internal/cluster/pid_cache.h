#ifndef PROTOACTOR_CLUSTER_PID_CACHE_H
#define PROTOACTOR_CLUSTER_PID_CACHE_H

#include "external/pid.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>

namespace protoactor {
namespace cluster {

// Forward declaration
class Member;

/**
 * @brief PidCache caches virtual actor PIDs.
 */
class PidCache {
public:
    PidCache();
    
    /**
     * @brief Get a PID from cache.
     * @param identity Identity string
     * @return PID or nullptr if not found
     */
    std::shared_ptr<PID> Get(const std::string& identity) const;
    
    /**
     * @brief Add a PID to cache.
     * @param identity Identity string
     * @param pid PID
     */
    void Add(const std::string& identity, std::shared_ptr<PID> pid);
    
    /**
     * @brief Remove a PID from cache.
     * @param identity Identity string
     */
    void Remove(const std::string& identity);
    
    /**
     * @brief Remove all PIDs for a member.
     * @param member Member
     */
    void RemoveByMember(std::shared_ptr<Member> member);
    
    /**
     * @brief Clear the cache.
     */
    void Clear();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<PID>> cache_;
};

} // namespace cluster
} // namespace protoactor

#endif // PROTOACTOR_CLUSTER_PID_CACHE_H
