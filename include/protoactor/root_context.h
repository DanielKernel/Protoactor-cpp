#ifndef PROTOACTOR_ROOT_CONTEXT_H
#define PROTOACTOR_ROOT_CONTEXT_H

#include "context.h"
#include <memory>

namespace protoactor {

// Forward declarations
class ActorSystem;

/**
 * @brief Root context for spawning top-level actors.
 */
class RootContext : public Context, public std::enable_shared_from_this<RootContext> {
public:
    explicit RootContext(std::shared_ptr<ActorSystem> actor_system);
    
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

private:
    std::shared_ptr<ActorSystem> actor_system_;
    std::shared_ptr<ReadonlyMessageHeader> message_header_;
};

} // namespace protoactor

#endif // PROTOACTOR_ROOT_CONTEXT_H
