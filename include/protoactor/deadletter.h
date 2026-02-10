#ifndef PROTOACTOR_DEADLETTER_H
#define PROTOACTOR_DEADLETTER_H

#include "process.h"
#include "pid.h"
#include <memory>

namespace protoactor {

// Forward declarations
class ActorSystem;

/**
 * @brief DeadLetterEvent is published when a message is sent to a nonexistent PID.
 */
class DeadLetterEvent {
public:
    std::shared_ptr<PID> pid;      // The invalid process
    std::shared_ptr<void> message; // The message that could not be delivered
    std::shared_ptr<PID> sender;  // The process that sent the message
    
    DeadLetterEvent(
        std::shared_ptr<PID> p,
        std::shared_ptr<void> msg,
        std::shared_ptr<PID> snd = nullptr)
        : pid(p), message(msg), sender(snd) {
    }
};

/**
 * @brief DeadLetterResponse is sent back to the sender when a message cannot be delivered.
 */
class DeadLetterResponse {
public:
    std::shared_ptr<PID> target;
    
    explicit DeadLetterResponse(std::shared_ptr<PID> t) : target(t) {}
};

/**
 * @brief DeadLetterProcess handles messages sent to non-existent actors.
 */
class DeadLetterProcess : public Process, public std::enable_shared_from_this<DeadLetterProcess> {
public:
    /**
     * @brief Create a new dead letter process.
     * @param actor_system The actor system
     * @return Dead letter process
     */
    static std::shared_ptr<DeadLetterProcess> New(std::shared_ptr<ActorSystem> actor_system);
    
    void SendUserMessage(std::shared_ptr<PID> pid, std::shared_ptr<void> message) override;
    void SendSystemMessage(std::shared_ptr<PID> pid, std::shared_ptr<void> message) override;
    void Stop(std::shared_ptr<PID> pid) override;

private:
    std::shared_ptr<ActorSystem> actor_system_;
    
public:
    explicit DeadLetterProcess(std::shared_ptr<ActorSystem> actor_system);
private:
    void Initialize();
};

} // namespace protoactor

#endif // PROTOACTOR_DEADLETTER_H
