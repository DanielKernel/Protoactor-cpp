#ifndef PROTOACTOR_ACTOR_CONTEXT_H
#define PROTOACTOR_ACTOR_CONTEXT_H

#include "protoactor/context.h"
#include "protoactor/actor.h"
#include "protoactor/props.h"
#include "protoactor/pid.h"
#include "protoactor/messages.h"
#include "protoactor/supervision.h"
#include "protoactor/internal/scheduler/timer.h" // Include scheduler timer header
#include <memory>
#include <vector>
#include <atomic>
#include <chrono>
#include <stack>

namespace protoactor {

// Forward declarations
class ActorSystem;
class RestartStatistics;
class CapturedContext;

/**
 * @brief ActorContext is the context implementation for actors.
 */
class ActorContext : public Context, public std::enable_shared_from_this<ActorContext> {
public:
    /**
     * @brief Create a new actor context.
     * @param actor_system The actor system
     * @param props Actor properties
     * @param parent Parent PID
     * @return Actor context
     */
    static std::shared_ptr<ActorContext> New(
        std::shared_ptr<ActorSystem> actor_system,
        std::shared_ptr<Props> props,
        std::shared_ptr<PID> parent);
    
    // Context interface implementation
    std::shared_ptr<PID> Parent() override;
    std::shared_ptr<PID> Self() override;
    std::shared_ptr<Actor> GetActor() override;
    std::shared_ptr<ActorSystem> GetActorSystem() override;
    std::shared_ptr<void> Message() override;
    std::shared_ptr<ReadonlyMessageHeader> MessageHeader() override;
    std::shared_ptr<PID> Sender() override;
    void Send(std::shared_ptr<PID> pid, std::shared_ptr<void> message) override;
    void Request(std::shared_ptr<PID> pid, std::shared_ptr<void> message) override;
    void RequestWithCustomSender(std::shared_ptr<PID> pid, std::shared_ptr<void> message, std::shared_ptr<PID> sender) override;
    std::shared_ptr<Future> RequestFuture(std::shared_ptr<PID> pid, std::shared_ptr<void> message, std::chrono::milliseconds timeout) override;
    std::chrono::milliseconds ReceiveTimeout() override;
    std::vector<std::shared_ptr<PID>> Children() override;
    void Respond(std::shared_ptr<void> response) override;
    void Stash() override;
    void Unstash() override;
    void UnstashAll() override;
    void Watch(std::shared_ptr<PID> pid) override;
    void Unwatch(std::shared_ptr<PID> pid) override;
    void SetReceiveTimeout(std::chrono::milliseconds timeout) override;
    void CancelReceiveTimeout() override;
    void Forward(std::shared_ptr<PID> pid) override;
    void ReenterAfter(std::shared_ptr<Future> future, std::function<void(std::shared_ptr<void>, std::error_code)> continuation) override;
    std::shared_ptr<CapturedContext> Capture() override;
    void Apply(std::shared_ptr<CapturedContext> captured) override;
    std::shared_ptr<PID> Spawn(std::shared_ptr<Props> props) override;
    std::shared_ptr<PID> SpawnPrefix(std::shared_ptr<Props> props, const std::string& prefix) override;
    std::pair<std::shared_ptr<PID>, std::error_code> SpawnNamed(std::shared_ptr<Props> props, const std::string& id) override;
    void Stop(std::shared_ptr<PID> pid) override;
    std::shared_ptr<Future> StopFuture(std::shared_ptr<PID> pid) override;
    void Poison(std::shared_ptr<PID> pid) override;
    std::shared_ptr<Future> PoisonFuture(std::shared_ptr<PID> pid) override;
    
    // MessageInvoker interface (for Mailbox)
    void InvokeUserMessage(std::shared_ptr<void> message);
    void InvokeSystemMessage(std::shared_ptr<void> message);
    void EscalateFailure(std::shared_ptr<void> reason, std::shared_ptr<void> message);
    
    // Internal: set self PID (called during spawn)
    void SetSelf(std::shared_ptr<PID> self);

private:
    enum State {
        STATE_ALIVE = 0,
        STATE_RESTARTING = 1,
        STATE_STOPPING = 2,
        STATE_STOPPED = 3
    };
    
    std::shared_ptr<Actor> actor_;
    std::shared_ptr<ActorSystem> actor_system_;
    std::shared_ptr<Props> props_;
    std::shared_ptr<PID> parent_;
    std::shared_ptr<PID> self_;  // Set during spawn
    std::chrono::milliseconds receive_timeout_;
    std::shared_ptr<void> message_or_envelope_;
    std::atomic<int> state_;
    
    // Extras (lazy initialized)
    struct Extras {
        std::vector<std::shared_ptr<PID>> children_;
        std::vector<std::shared_ptr<PID>> watchers_;
        std::stack<std::shared_ptr<void>> stash_;
        std::shared_ptr<RestartStatistics> restart_stats_;
        std::shared_ptr<protoactor::scheduler::CancelFunc> receive_timeout_timer_; // Cancel function for receive timeout timer
    };
    std::unique_ptr<Extras> extras_;
    
    ActorContext(
        std::shared_ptr<ActorSystem> actor_system,
        std::shared_ptr<Props> props,
        std::shared_ptr<PID> parent);
    
    void IncarnateActor();
    Extras* EnsureExtras();
    void ProcessMessage(std::shared_ptr<void> message);
    void DefaultReceive();
    void HandleWatch(std::shared_ptr<protoactor::Watch> msg);
    void HandleUnwatch(std::shared_ptr<protoactor::Unwatch> msg);
    void HandleStop();
    void HandleTerminated(std::shared_ptr<protoactor::Terminated> msg);
    void HandleFailure(std::shared_ptr<protoactor::Failure> msg);
    void HandleRestart();
    void HandleContinuation(std::shared_ptr<Continuation> msg);
    void SendUserMessage(std::shared_ptr<PID> pid, std::shared_ptr<void> message);
    void ReceiveTimeoutHandler();
};

} // namespace protoactor

#endif // PROTOACTOR_ACTOR_CONTEXT_H
