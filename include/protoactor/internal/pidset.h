#ifndef PROTOACTOR_PIDSET_H
#define PROTOACTOR_PIDSET_H

#include "protoactor/pid.h"
#include <unordered_set>
#include <vector>
#include <memory>

namespace protoactor {

/**
 * @brief PIDSet is a set of PIDs for managing multiple actor references.
 * 
 * This is useful for tracking multiple actors, such as routees in a router.
 */
class PIDSet {
public:
    /**
     * @brief Create a new empty PIDSet.
     * @return PIDSet instance
     */
    static std::shared_ptr<PIDSet> New();
    
    /**
     * @brief Add a PID to the set.
     * @param pid PID to add
     */
    void Add(std::shared_ptr<PID> pid);
    
    /**
     * @brief Remove a PID from the set.
     * @param pid PID to remove
     */
    void Remove(std::shared_ptr<PID> pid);
    
    /**
     * @brief Check if a PID is in the set.
     * @param pid PID to check
     * @return true if present, false otherwise
     */
    bool Contains(std::shared_ptr<PID> pid) const;
    
    /**
     * @brief Get all PIDs in the set.
     * @return Vector of all PIDs
     */
    std::vector<std::shared_ptr<PID>> GetAll() const;
    
    /**
     * @brief Get the number of PIDs in the set.
     * @return Number of PIDs
     */
    size_t Len() const;
    
    /**
     * @brief Clear all PIDs from the set.
     */
    void Clear();
    
    /**
     * @brief Check if the set is empty.
     * @return true if empty, false otherwise
     */
    bool Empty() const;

private:
    // Custom hash function for PID
    struct PIDHash {
        size_t operator()(const std::shared_ptr<PID>& pid) const {
            if (!pid) return 0;
            std::hash<std::string> hasher;
            return hasher(pid->address + ":" + pid->id);
        }
    };
    
    // Custom equality function for PID
    struct PIDEqual {
        bool operator()(const std::shared_ptr<PID>& a, const std::shared_ptr<PID>& b) const {
            if (!a || !b) return a == b;
            return a->address == b->address && a->id == b->id;
        }
    };
    
    std::unordered_set<std::shared_ptr<PID>, PIDHash, PIDEqual> pids_;
    
    PIDSet() = default;
};

} // namespace protoactor

#endif // PROTOACTOR_PIDSET_H
