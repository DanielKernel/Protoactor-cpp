#ifndef PROTOACTOR_ACTOR_SYSTEM_H
#define PROTOACTOR_ACTOR_SYSTEM_H

#include "context.h"
#include "process_registry.h"
#include "root_context.h"
#include <memory>
#include <string>
#include <chrono>

namespace protoactor {

// Forward declarations
class DeadLetterProcess;
class Guardians;
class Extensions;
class Config;
class ProcessRegistry;

namespace eventstream {
class EventStream;
}

/**
 * @brief ActorSystem is the runtime environment that hosts actors and manages their
 * execution, supervision, and system-wide services.
 */
class ActorSystem : public std::enable_shared_from_this<ActorSystem> {
public:
    /**
     * @brief Create a new actor system with optional configuration.
     * @param config Configuration options
     * @return Actor system instance
     */
    static std::shared_ptr<ActorSystem> New(std::shared_ptr<Config> config = nullptr);
    
    /**
     * @brief Get the process registry.
     * @return Process registry
     */
    std::shared_ptr<ProcessRegistry> GetProcessRegistry() const;
    
    /**
     * @brief Get the root context.
     * @return Root context
     */
    std::shared_ptr<RootContext> GetRoot() const;
    
    /**
     * @brief Get the event stream.
     * @return Event stream
     */
    std::shared_ptr<eventstream::EventStream> GetEventStream() const;
    
    /**
     * @brief Get the dead letter process.
     * @return Dead letter process
     */
    std::shared_ptr<DeadLetterProcess> GetDeadLetter() const;
    
    /**
     * @brief Get the extensions.
     * @return Extensions
     */
    std::shared_ptr<Extensions> GetExtensions() const;
    
    /**
     * @brief Get the system ID.
     * @return System ID
     */
    std::string GetID() const;
    
    /**
     * @brief Get the network address.
     * @return Address string
     */
    std::string Address() const;
    
    /**
     * @brief Create a new local PID with the given id.
     * @param id Identifier
     * @return New PID
     */
    std::shared_ptr<PID> NewLocalPID(const std::string& id);
    
    /**
     * @brief Shutdown the actor system.
     */
    void Shutdown();
    
    /**
     * @brief Check if the system is stopped.
     * @return true if stopped
     */
    bool IsStopped() const;

private:
    std::shared_ptr<ProcessRegistry> process_registry_;
    std::shared_ptr<RootContext> root_;
    std::shared_ptr<eventstream::EventStream> event_stream_;
    std::shared_ptr<Guardians> guardians_;
    std::shared_ptr<DeadLetterProcess> dead_letter_;
    std::shared_ptr<Extensions> extensions_;
    std::shared_ptr<Config> config_;
    std::string id_;
    std::atomic<bool> stopped_;
    
public:
    ActorSystem();
private:
    void Initialize(std::shared_ptr<Config> config);
};

} // namespace protoactor

#endif // PROTOACTOR_ACTOR_SYSTEM_H
