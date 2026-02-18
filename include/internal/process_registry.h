#ifndef PROTOACTOR_PROCESS_REGISTRY_H
#define PROTOACTOR_PROCESS_REGISTRY_H

#include "external/pid.h"
#include "internal/process.h"
#include <memory>
#include <string>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <array>
#include <vector>

namespace protoactor {

// Forward declarations
class ActorSystem;

/**
 * @brief Address resolver function type for resolving remote actors.
 */
using AddressResolver = std::function<std::pair<std::shared_ptr<Process>, bool>(std::shared_ptr<PID>)>;

/**
 * @brief ProcessRegistry tracks processes within an actor system.
 */
class ProcessRegistry {
public:
    /**
     * @brief Create a new process registry.
     * @param actor_system The actor system
     * @return Process registry instance
     */
    static std::shared_ptr<ProcessRegistry> New(std::shared_ptr<ActorSystem> actor_system);
    
    /**
     * @brief Get the next unique process identifier.
     * @return Next ID string
     */
    std::string NextID();
    
    /**
     * @brief Add a process to the registry.
     * @param process The process
     * @param id Identifier
     * @return PID and whether it was newly added
     */
    std::pair<std::shared_ptr<PID>, bool> Add(std::shared_ptr<Process> process, const std::string& id);
    
    /**
     * @brief Remove a process from the registry.
     * @param pid The PID to remove
     */
    void Remove(std::shared_ptr<PID> pid);
    
    /**
     * @brief Get a process by PID.
     * @param pid The PID
     * @return Process and whether it was found
     */
    std::pair<std::shared_ptr<Process>, bool> Get(std::shared_ptr<PID> pid);
    
    /**
     * @brief Get a local process by id.
     * @param id The identifier
     * @return Process and whether it was found
     */
    std::pair<std::shared_ptr<Process>, bool> GetLocal(const std::string& id);

    /**
     * @brief Clear all processes from the registry.
     */
    void Clear();

    /**
     * @brief Register an address resolver for remote addresses.
     * @param resolver Resolver function
     */
    void RegisterAddressResolver(AddressResolver resolver);
    
    /**
     * @brief Get the address of this registry.
     * @return Address string
     */
    std::string Address() const;

private:
    std::atomic<uint64_t> sequence_id_;
    std::shared_ptr<ActorSystem> actor_system_;
    std::string address_;
    
    // Sharded map for local PIDs
    static constexpr int NUM_SHARDS = 1024;
    std::array<std::unordered_map<std::string, std::shared_ptr<Process>>, NUM_SHARDS> local_pids_;
    std::array<std::mutex, NUM_SHARDS> shard_mutexes_;
    
    std::vector<AddressResolver> remote_handlers_;
    
public:
    ProcessRegistry(std::shared_ptr<ActorSystem> actor_system);
    
private:
    /**
     * @brief Get the shard bucket for a key.
     * @param key The key
     * @return Shard index
     */
    int GetBucket(const std::string& key) const;
    
    /**
     * @brief Convert uint64 to ID string.
     * @param u The number
     * @return ID string
     */
    static std::string Uint64ToID(uint64_t u);
};

} // namespace protoactor

#endif // PROTOACTOR_PROCESS_REGISTRY_H
