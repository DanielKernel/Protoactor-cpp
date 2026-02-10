#include "protoactor/context.h"
#include "protoactor/root_context.h"
#include "protoactor/actor_system.h"
#include "protoactor/future.h"
#include "protoactor/messages.h"
#include "protoactor/pid.h"
#include "protoactor/props.h"
#include "protoactor/new_pid.h"
#include <stdexcept>

// Forward declaration
namespace protoactor {
std::shared_ptr<ReadonlyMessageHeader> EmptyMessageHeader();
}

namespace protoactor {

RootContext::RootContext(std::shared_ptr<ActorSystem> actor_system)
    : actor_system_(actor_system), message_header_(EmptyMessageHeader()) {
}

std::shared_ptr<PID> RootContext::Parent() {
    return nullptr;
}

std::shared_ptr<PID> RootContext::Self() {
    return nullptr;
}

std::shared_ptr<Actor> RootContext::GetActor() {
    return nullptr;
}

std::shared_ptr<ActorSystem> RootContext::GetActorSystem() {
    return actor_system_;
}

std::shared_ptr<void> RootContext::Message() {
    return nullptr;
}

std::shared_ptr<ReadonlyMessageHeader> RootContext::MessageHeader() {
    return message_header_;
}

std::shared_ptr<PID> RootContext::Sender() {
    return nullptr;
}

void RootContext::Send(std::shared_ptr<PID> pid, std::shared_ptr<void> message) {
    if (pid) {
        // Wrap message in envelope for consistency with ActorContext::SendUserMessage
        auto env = std::make_shared<MessageEnvelope>(nullptr, message, nullptr);
        pid->SendUserMessage(actor_system_, env);
    }
}

void RootContext::Request(std::shared_ptr<PID> pid, std::shared_ptr<void> message) {
    if (pid) {
        auto env = std::make_shared<MessageEnvelope>(nullptr, message, nullptr);
        pid->SendUserMessage(actor_system_, env);
    }
}

void RootContext::RequestWithCustomSender(
    std::shared_ptr<PID> pid,
    std::shared_ptr<void> message,
    std::shared_ptr<PID> sender) {
    if (pid) {
        auto env = std::make_shared<MessageEnvelope>(nullptr, message, sender);
        pid->SendUserMessage(actor_system_, env);
    }
}

std::shared_ptr<Future> RootContext::RequestFuture(
    std::shared_ptr<PID> pid,
    std::shared_ptr<void> message,
    std::chrono::milliseconds timeout) {
    auto future = NewFuture(actor_system_, timeout);
    auto env = std::make_shared<MessageEnvelope>(nullptr, message, future->GetPID());
    pid->SendUserMessage(actor_system_, env);
    return future;
}

std::chrono::milliseconds RootContext::ReceiveTimeout() {
    return std::chrono::milliseconds(0);
}

std::vector<std::shared_ptr<PID>> RootContext::Children() {
    return std::vector<std::shared_ptr<PID>>();
}

void RootContext::Respond(std::shared_ptr<void> response) {
    // Root context has no sender
}

void RootContext::Stash() {
    // Not applicable for root context
}

void RootContext::Unstash() {
    // Not applicable for root context
}

void RootContext::UnstashAll() {
    // Not applicable for root context
}

void RootContext::Watch(std::shared_ptr<PID> pid) {
    if (pid) {
        auto watch_msg = std::make_shared<protoactor::Watch>(nullptr);
        pid->SendSystemMessage(actor_system_, watch_msg);
    }
}

void RootContext::Unwatch(std::shared_ptr<PID> pid) {
    if (pid) {
        auto unwatch_msg = std::make_shared<protoactor::Unwatch>(nullptr);
        pid->SendSystemMessage(actor_system_, unwatch_msg);
    }
}

void RootContext::SetReceiveTimeout(std::chrono::milliseconds timeout) {
    // Not applicable for root context
}

void RootContext::CancelReceiveTimeout() {
    // Not applicable for root context
}

void RootContext::Forward(std::shared_ptr<PID> pid) {
    // Not applicable for root context
}

void RootContext::ReenterAfter(
    std::shared_ptr<Future> future,
    std::function<void(std::shared_ptr<void>, std::error_code)> continuation) {
    // Not applicable for root context
}

std::shared_ptr<CapturedContext> RootContext::Capture() {
    return nullptr;
}

void RootContext::Apply(std::shared_ptr<CapturedContext> captured) {
    // Not applicable for root context
}

std::shared_ptr<PID> RootContext::Spawn(std::shared_ptr<Props> props) {
    auto id = actor_system_->GetProcessRegistry()->NextID();
    auto [pid, err] = SpawnNamed(props, id);
    if (err) {
        throw std::runtime_error("Failed to spawn actor: " + err.message());
    }
    return pid;
}

std::shared_ptr<PID> RootContext::SpawnPrefix(std::shared_ptr<Props> props, const std::string& prefix) {
    auto id = prefix + actor_system_->GetProcessRegistry()->NextID();
    auto [pid, err] = SpawnNamed(props, id);
    if (err) {
        throw std::runtime_error("Failed to spawn actor: " + err.message());
    }
    return pid;
}

std::pair<std::shared_ptr<PID>, std::error_code> RootContext::SpawnNamed(
    std::shared_ptr<Props> props,
    const std::string& id) {
    return props->Spawn(actor_system_, id, shared_from_this());
}

void RootContext::Stop(std::shared_ptr<PID> pid) {
    if (pid) {
        auto process = pid->Ref(actor_system_);
        if (process) {
            process->Stop(pid);
        }
    }
}

std::shared_ptr<Future> RootContext::StopFuture(std::shared_ptr<PID> pid) {
    auto future = NewFuture(actor_system_, std::chrono::milliseconds(10000));
    auto watch_msg = std::make_shared<protoactor::Watch>(future->GetPID());
    pid->SendSystemMessage(actor_system_, watch_msg);
    Stop(pid);
    return future;
}

void RootContext::Poison(std::shared_ptr<PID> pid) {
    if (pid) {
        auto poison = std::make_shared<PoisonPill>();
        pid->SendUserMessage(actor_system_, poison);
    }
}

std::shared_ptr<Future> RootContext::PoisonFuture(std::shared_ptr<PID> pid) {
    auto future = NewFuture(actor_system_, std::chrono::milliseconds(10000));
    auto watch_msg = std::make_shared<protoactor::Watch>(future->GetPID());
    pid->SendSystemMessage(actor_system_, watch_msg);
    Poison(pid);
    return future;
}

} // namespace protoactor
