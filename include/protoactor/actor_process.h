#ifndef PROTOACTOR_ACTOR_PROCESS_H
#define PROTOACTOR_ACTOR_PROCESS_H

#include "process.h"
#include "mailbox.h"
#include "pid.h"
#include <memory>
#include <atomic>

namespace protoactor {

// Forward declarations
class MessageInvoker;

/**
 * @brief ActorProcess is the Process implementation for regular actors.
 */
class ActorProcess : public Process, public std::enable_shared_from_this<ActorProcess> {
public:
    /**
     * @brief Create a new ActorProcess with the given mailbox.
     * @param mailbox The mailbox
     * @return ActorProcess instance
     */
    static std::shared_ptr<ActorProcess> New(std::shared_ptr<Mailbox> mailbox);
    
    /**
     * @brief Get the mailbox.
     * @return Mailbox
     */
    std::shared_ptr<Mailbox> GetMailbox() const;
    
    /**
     * @brief Check if the process is dead.
     * @return true if dead
     */
    bool IsDead() const;
    
    void SendUserMessage(std::shared_ptr<PID> pid, std::shared_ptr<void> message) override;
    void SendSystemMessage(std::shared_ptr<PID> pid, std::shared_ptr<void> message) override;
    void Stop(std::shared_ptr<PID> pid) override;

private:
    std::shared_ptr<Mailbox> mailbox_;
    std::atomic<int> dead_;
    
public:
    explicit ActorProcess(std::shared_ptr<Mailbox> mailbox);
};

} // namespace protoactor

#endif // PROTOACTOR_ACTOR_PROCESS_H
