#ifndef PROTOACTOR_CONTEXT_H
#define PROTOACTOR_CONTEXT_H

#include "pid.h"
#include "actor.h"
#include "future.h"
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <system_error>

// Forward declarations
class CapturedContext;

namespace protoactor {

// Forward declarations
class ActorSystem;
class MessageHeader;
class ReadonlyMessageHeader;
struct MessageEnvelope;
class CapturedContext;
class Props;

/**
 * @brief Context contains contextual information for actors.
 * 
 * This is the main interface actors use to interact with the actor system.
 */
class Context {
public:
    virtual ~Context() = default;
    
    // Info part
    /**
     * @brief Get the PID of the current actor's parent.
     * @return Parent PID or nullptr
     */
    virtual std::shared_ptr<PID> Parent() = 0;
    
    /**
     * @brief Get the PID of the current actor.
     * @return Self PID
     */
    virtual std::shared_ptr<PID> Self() = 0;
    
    /**
     * @brief Get the actor instance associated with this context.
     * @return Actor instance
     */
    virtual std::shared_ptr<Actor> GetActor() = 0;
    
    /**
     * @brief Get the actor system.
     * @return Actor system instance
     */
    virtual std::shared_ptr<ActorSystem> GetActorSystem() = 0;
    
    // Message part
    /**
     * @brief Get the current message being processed.
     * @return The message (as void* for type erasure)
     */
    virtual std::shared_ptr<void> Message() = 0;
    
    /**
     * @brief Get the message header (metadata).
     * @return Message header
     */
    virtual std::shared_ptr<ReadonlyMessageHeader> MessageHeader() = 0;
    
    // Sender part
    /**
     * @brief Get the PID of the actor that sent the current message.
     * @return Sender PID or nullptr
     */
    virtual std::shared_ptr<PID> Sender() = 0;
    
    /**
     * @brief Send a message to the given PID.
     * @param pid Target PID
     * @param message The message to send
     */
    virtual void Send(std::shared_ptr<PID> pid, std::shared_ptr<void> message) = 0;
    
    /**
     * @brief Send a request message (includes sender PID).
     * @param pid Target PID
     * @param message The message to send
     */
    virtual void Request(std::shared_ptr<PID> pid, std::shared_ptr<void> message) = 0;
    
    /**
     * @brief Send a request with custom sender PID.
     * @param pid Target PID
     * @param message The message to send
     * @param sender Custom sender PID
     */
    virtual void RequestWithCustomSender(std::shared_ptr<PID> pid, std::shared_ptr<void> message, std::shared_ptr<PID> sender) = 0;
    
    /**
     * @brief Send a request and get a Future for the response.
     * @param pid Target PID
     * @param message The message to send
     * @param timeout Timeout duration
     * @return Future for the response
     */
    virtual std::shared_ptr<Future> RequestFuture(std::shared_ptr<PID> pid, std::shared_ptr<void> message, std::chrono::milliseconds timeout) = 0;
    
    // Base part
    /**
     * @brief Get the receive timeout duration.
     * @return Timeout duration
     */
    virtual std::chrono::milliseconds ReceiveTimeout() = 0;
    
    /**
     * @brief Get the children of this actor.
     * @return Vector of child PIDs
     */
    virtual std::vector<std::shared_ptr<PID>> Children() = 0;
    
    /**
     * @brief Send a response to the current sender.
     * @param response The response message
     */
    virtual void Respond(std::shared_ptr<void> response) = 0;
    
    /**
     * @brief Stash the current message for later processing.
     */
    virtual void Stash() = 0;
    
    /**
     * @brief Unstash the next stashed message (if any).
     */
    virtual void Unstash() = 0;
    
    /**
     * @brief Unstash all stashed messages.
     */
    virtual void UnstashAll() = 0;
    
    /**
     * @brief Watch the specified PID (receive Terminated when it stops).
     * @param pid PID to watch
     */
    virtual void Watch(std::shared_ptr<PID> pid) = 0;
    
    /**
     * @brief Unwatch the specified PID.
     * @param pid PID to unwatch
     */
    virtual void Unwatch(std::shared_ptr<PID> pid) = 0;
    
    /**
     * @brief Set the receive timeout.
     * @param timeout Timeout duration
     */
    virtual void SetReceiveTimeout(std::chrono::milliseconds timeout) = 0;
    
    /**
     * @brief Cancel the receive timeout.
     */
    virtual void CancelReceiveTimeout() = 0;
    
    /**
     * @brief Forward the current message to the given PID.
     * @param pid Target PID
     */
    virtual void Forward(std::shared_ptr<PID> pid) = 0;
    
    /**
     * @brief Reenter after a future completes.
     * @param future The future to wait for
     * @param continuation Continuation function
     */
    virtual void ReenterAfter(std::shared_ptr<Future> future, std::function<void(std::shared_ptr<void>, std::error_code)> continuation) = 0;
    
    /**
     * @brief Capture the current message envelope.
     * @return Captured context
     */
    virtual std::shared_ptr<CapturedContext> Capture() = 0;
    
    /**
     * @brief Apply a captured context to restore state.
     * @param captured The captured context
     */
    virtual void Apply(std::shared_ptr<CapturedContext> captured) = 0;
    
    // Spawner part
    /**
     * @brief Spawn a new child actor.
     * @param props Actor properties
     * @return The spawned PID
     */
    virtual std::shared_ptr<PID> Spawn(std::shared_ptr<Props> props) = 0;
    
    /**
     * @brief Spawn a new child actor with a prefix.
     * @param props Actor properties
     * @param prefix Name prefix
     * @return The spawned PID
     */
    virtual std::shared_ptr<PID> SpawnPrefix(std::shared_ptr<Props> props, const std::string& prefix) = 0;
    
    /**
     * @brief Spawn a new child actor with a specific name.
     * @param props Actor properties
     * @param id The name
     * @return The spawned PID and error code
     */
    virtual std::pair<std::shared_ptr<PID>, std::error_code> SpawnNamed(std::shared_ptr<Props> props, const std::string& id) = 0;
    
    // Stopper part
    /**
     * @brief Stop an actor immediately.
     * @param pid PID to stop
     */
    virtual void Stop(std::shared_ptr<PID> pid) = 0;
    
    /**
     * @brief Stop an actor and get a future.
     * @param pid PID to stop
     * @return Future for completion
     */
    virtual std::shared_ptr<Future> StopFuture(std::shared_ptr<PID> pid) = 0;
    
    /**
     * @brief Poison an actor (stop after processing current messages).
     * @param pid PID to poison
     */
    virtual void Poison(std::shared_ptr<PID> pid) = 0;
    
    /**
     * @brief Poison an actor and get a future.
     * @param pid PID to poison
     * @return Future for completion
     */
    virtual std::shared_ptr<Future> PoisonFuture(std::shared_ptr<PID> pid) = 0;
};

// Forward declaration
class CapturedContext;

// RootContext is declared in root_context.h

} // namespace protoactor

#endif // PROTOACTOR_CONTEXT_H
