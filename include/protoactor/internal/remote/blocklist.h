#ifndef PROTOACTOR_REMOTE_BLOCKLIST_H
#define PROTOACTOR_REMOTE_BLOCKLIST_H

#include <string>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <memory>
#ifdef __cpp_lib_shared_mutex
#include <shared_mutex> // For std::shared_mutex (C++14+)
#else
// Use std::mutex as fallback for C++11
namespace std {
    using shared_mutex = mutex;
    template<typename T>
    using shared_lock = lock_guard<T>;
}
#endif

namespace protoactor {
namespace remote {

/**
 * @brief BlockList keeps track of blocked cluster member IDs.
 * 
 * This is used to prevent connections from specific cluster members.
 */
class BlockList {
public:
    /**
     * @brief Create a new empty BlockList.
     * @return BlockList instance
     */
    static std::shared_ptr<BlockList> New();
    
    /**
     * @brief Block adds the given member IDs to the BlockList.
     * @param member_ids Member IDs to block
     */
    void Block(const std::vector<std::string>& member_ids);
    
    /**
     * @brief Block adds a single member ID to the BlockList.
     * @param member_id Member ID to block
     */
    void Block(const std::string& member_id);
    
    /**
     * @brief IsBlocked returns true if the given member ID is blocked.
     * @param member_id Member ID to check
     * @return true if blocked, false otherwise
     */
    bool IsBlocked(const std::string& member_id) const;
    
    /**
     * @brief BlockedMembers returns the set of blocked member IDs.
     * @return Vector of blocked member IDs
     */
    std::vector<std::string> BlockedMembers() const;
    
    /**
     * @brief Len returns the number of blocked members.
     * @return Number of blocked members
     */
    size_t Len() const;

private:
    mutable std::shared_mutex mutex_;
    std::unordered_set<std::string> blocked_members_;
    
    BlockList() = default;
};

} // namespace remote
} // namespace protoactor

#endif // PROTOACTOR_REMOTE_BLOCKLIST_H
