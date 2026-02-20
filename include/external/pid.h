#ifndef PROTOACTOR_PID_H
#define PROTOACTOR_PID_H

#include <string>
#include <cstdint>
#include <memory>
#include <atomic>

namespace protoactor {

// Forward declarations
class Process;
class ActorSystem;

/**
 * @brief Process Identifier (PID) uniquely identifies an actor instance.
 * 
 * PID contains the address and id of an actor, and can be used to send messages.
 */
class PID : public std::enable_shared_from_this<PID> {
public:
    std::string address;      // Network address of the actor system
    std::string id;           // Unique identifier within the address
    std::uint32_t request_id; // Request ID for request-response patterns
    
    /**
     * @brief Construct a new PID.
     * @param addr The network address
     * @param identifier The unique identifier
     */
    PID(const std::string& addr = "", const std::string& identifier = "");
    
    /**
     * @brief Check if two PIDs refer to the same actor instance.
     * @param other The other PID to compare
     * @return true if equal, false otherwise
     */
    bool Equal(const std::shared_ptr<PID>& other) const;
    
    /**
     * @brief Get or resolve the process reference for this PID.
     * @param actor_system The actor system to resolve from
     * @return The process reference
     */
    std::shared_ptr<Process> Ref(std::shared_ptr<ActorSystem> actor_system);
    
    /**
     * @brief Send a user message to this PID.
     * @param actor_system The actor system
     * @param message The message to send
     */
    void SendUserMessage(std::shared_ptr<ActorSystem> actor_system, std::shared_ptr<void> message);
    
    /**
     * @brief Send a system message to this PID.
     * @param actor_system The actor system
     * @param message The message to send
     */
    void SendSystemMessage(std::shared_ptr<ActorSystem> actor_system, std::shared_ptr<void> message);

    /**
     * @brief Clear the cached process pointer. Call when the process is stopped/removed.
     */
    void ClearCache();

private:
    std::atomic<Process*> process_ptr_; // Cached process pointer (atomic for thread safety)
};

/**
 * @brief Create a new PID with the given address and id.
 * @param address The network address
 * @param id The unique identifier
 * @return A new PID instance
 */
std::shared_ptr<PID> NewPID(const std::string& address, const std::string& id);

} // namespace protoactor

#endif // PROTOACTOR_PID_H
