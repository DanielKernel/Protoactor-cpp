#include "internal/actor/actor_context.h"
#include "external/actor_system.h"
#include "external/props.h"
#include "external/messages.h"
#include "external/future.h"
#include "external/supervision.h"
#include "internal/actor/captured_context.h"
#include "internal/actor/new_pid.h"
#include "internal/scheduler/timer.h"
#include "internal/actor/deadletter.h" // Include DeadLetterProcess header
#include <stdexcept>
#include <algorithm>

namespace protoactor {

ActorContext::ActorContext(
    std::shared_ptr<ActorSystem> actor_system,
    std::shared_ptr<Props> props,
    std::shared_ptr<PID> parent)
    : actor_system_(actor_system),
      props_(props),
      parent_(parent),
      receive_timeout_(std::chrono::milliseconds(0)),
      state_(STATE_ALIVE) {
    IncarnateActor();
}

std::shared_ptr<ActorContext> ActorContext::New(
    std::shared_ptr<ActorSystem> actor_system,
    std::shared_ptr<Props> props,
    std::shared_ptr<PID> parent) {
    return std::shared_ptr<ActorContext>(new ActorContext(actor_system, props, parent));
}

void ActorContext::IncarnateActor() {
    state_.store(STATE_ALIVE, std::memory_order_release);
    auto producer = props_->GetProducer();
    actor_ = producer(actor_system_);
}

ActorContext::Extras* ActorContext::EnsureExtras() {
    if (!extras_) {
        extras_.reset(new Extras());
    }
    return extras_.get();
}

std::shared_ptr<PID> ActorContext::Parent() {
    return parent_;
}

std::shared_ptr<PID> ActorContext::Self() {
    return self_;
}

std::shared_ptr<Actor> ActorContext::GetActor() {
    return actor_;
}

std::shared_ptr<ActorSystem> ActorContext::GetActorSystem() {
    return actor_system_;
}

std::shared_ptr<void> ActorContext::Message() {
    auto [header, msg, sender] = UnwrapEnvelope(message_or_envelope_);
    return msg;
}

std::shared_ptr<ReadonlyMessageHeader> ActorContext::MessageHeader() {
    auto [header, msg, sender] = UnwrapEnvelope(message_or_envelope_);
    return header;
}

std::shared_ptr<PID> ActorContext::Sender() {
    auto [header, msg, sender] = UnwrapEnvelope(message_or_envelope_);
    return sender;
}

void ActorContext::Send(std::shared_ptr<PID> pid, std::shared_ptr<void> message) {
    SendUserMessage(pid, message);
}

void ActorContext::SendUserMessage(std::shared_ptr<PID> pid, std::shared_ptr<void> message) {
    if (!pid) {
        return;
    }
    
    // Wrap message in envelope if needed
    // Since message is void*, we can't use dynamic_cast
    // For now, always create a new envelope
    std::shared_ptr<MessageEnvelope> envelope = std::make_shared<MessageEnvelope>(nullptr, message, nullptr);
    
    // Use sender middleware chain if available
    if (props_ && props_->sender_middleware_chain_) {
        props_->sender_middleware_chain_(shared_from_this(), pid, envelope);
    } else {
        pid->SendUserMessage(actor_system_, envelope);
    }
}

void ActorContext::Request(std::shared_ptr<PID> pid, std::shared_ptr<void> message) {
    auto env = std::make_shared<MessageEnvelope>(nullptr, message, self_);
    SendUserMessage(pid, env);
}

void ActorContext::RequestWithCustomSender(
    std::shared_ptr<PID> pid,
    std::shared_ptr<void> message,
    std::shared_ptr<PID> sender) {
    auto env = std::make_shared<MessageEnvelope>(nullptr, message, sender);
    SendUserMessage(pid, env);
}

std::shared_ptr<Future> ActorContext::RequestFuture(
    std::shared_ptr<PID> pid,
    std::shared_ptr<void> message,
    std::chrono::milliseconds timeout) {
    auto future = NewFuture(actor_system_, timeout);
    auto env = std::make_shared<MessageEnvelope>(nullptr, message, future->GetPID());
    SendUserMessage(pid, env);
    return future;
}

std::chrono::milliseconds ActorContext::ReceiveTimeout() {
    return receive_timeout_;
}

std::vector<std::shared_ptr<PID>> ActorContext::Children() {
    if (!extras_) {
        return std::vector<std::shared_ptr<PID>>();
    }
    return extras_->children_;
}

void ActorContext::Respond(std::shared_ptr<void> response) {
    auto sender = Sender();
    if (!sender) {
        actor_system_->GetDeadLetter()->SendUserMessage(nullptr, response);
        return;
    }
    Send(sender, response);
}

void ActorContext::Stash() {
    auto extras = EnsureExtras();
    extras->stash_.push(message_or_envelope_);
}

void ActorContext::Unstash() {
    if (!extras_ || extras_->stash_.empty()) {
        return;
    }
    auto msg = extras_->stash_.top();
    extras_->stash_.pop();
    InvokeUserMessage(msg);
}

void ActorContext::UnstashAll() {
    if (!extras_ || extras_->stash_.empty()) {
        return;
    }
    while (!extras_->stash_.empty()) {
        auto msg = extras_->stash_.top();
        extras_->stash_.pop();
        InvokeUserMessage(msg);
    }
}

void ActorContext::Watch(std::shared_ptr<PID> pid) {
    if (pid) {
        auto watch_msg = std::make_shared<protoactor::Watch>(self_);
        pid->SendSystemMessage(actor_system_, watch_msg);
    }
}

void ActorContext::Unwatch(std::shared_ptr<PID> pid) {
    if (pid) {
        auto unwatch_msg = std::make_shared<protoactor::Unwatch>(self_);
        pid->SendSystemMessage(actor_system_, unwatch_msg);
    }
}

void ActorContext::SetReceiveTimeout(std::chrono::milliseconds timeout) {
    if (timeout < std::chrono::milliseconds(1)) {
        timeout = std::chrono::milliseconds(0);
    }
    
    if (timeout == receive_timeout_) {
        return;
    }
    
    receive_timeout_ = timeout;
    auto extras = EnsureExtras();
    
    // Cancel existing timer if any
    if (extras->receive_timeout_timer_) {
        auto& cancel_func = *extras->receive_timeout_timer_;
        if (cancel_func) {
            cancel_func();
        }
        extras->receive_timeout_timer_.reset();
    }
    
    // Setup new timer if timeout > 0
    if (timeout.count() > 0) {
        auto ctx = std::static_pointer_cast<protoactor::Context>(shared_from_this());
        auto timer_scheduler = protoactor::scheduler::TimerScheduler::New(ctx);
        auto timeout_msg = std::make_shared<protoactor::ReceiveTimeout>();
        auto cancel = timer_scheduler->SendOnce(timeout, self_, timeout_msg);
        extras->receive_timeout_timer_ = std::make_shared<protoactor::scheduler::CancelFunc>(std::move(cancel));
    }
}

void ActorContext::CancelReceiveTimeout() {
    if (!extras_) {
        return;
    }
    
    if (extras_->receive_timeout_timer_) {
        auto& cancel_func = *extras_->receive_timeout_timer_;
        if (cancel_func) {
            cancel_func();
        }
        extras_->receive_timeout_timer_.reset();
    }
    
    receive_timeout_ = std::chrono::milliseconds(0);
}

void ActorContext::Forward(std::shared_ptr<PID> pid) {
    // System messages cannot be forwarded
    auto sys_msg = std::static_pointer_cast<protoactor::SystemMessage>(message_or_envelope_);
    if (sys_msg) {
        return;
    }
    SendUserMessage(pid, message_or_envelope_);
}

void ActorContext::ReenterAfter(
    std::shared_ptr<Future> future,
    std::function<void(std::shared_ptr<void>, std::error_code)> continuation) {
    // Store current message
    auto msg = message_or_envelope_;
    
    // Create continuation message
    auto cont_msg = std::make_shared<Continuation>(continuation, msg);
    
    // Set up future continuation to send continuation message to self
    future->ContinueWith([this, cont_msg](std::shared_ptr<void> res, std::error_code err) {
        // Send continuation message to self
        self_->SendSystemMessage(actor_system_, cont_msg);
    });
}

std::shared_ptr<CapturedContext> ActorContext::Capture() {
    auto envelope = WrapEnvelope(message_or_envelope_);
    return std::make_shared<CapturedContext>(envelope, shared_from_this());
}

void ActorContext::Apply(std::shared_ptr<CapturedContext> captured) {
    if (captured) {
        message_or_envelope_ = captured->message_envelope;
    }
}

std::shared_ptr<PID> ActorContext::Spawn(std::shared_ptr<Props> props) {
    auto id = actor_system_->GetProcessRegistry()->NextID();
    auto [pid, err] = SpawnNamed(props, self_->id + "/" + id);
    if (err) {
        throw std::runtime_error("Failed to spawn actor");
    }
    return pid;
}

std::shared_ptr<PID> ActorContext::SpawnPrefix(std::shared_ptr<Props> props, const std::string& prefix) {
    auto id = prefix + actor_system_->GetProcessRegistry()->NextID();
    auto [pid, err] = SpawnNamed(props, self_->id + "/" + id);
    if (err) {
        throw std::runtime_error("Failed to spawn actor");
    }
    return pid;
}

std::pair<std::shared_ptr<PID>, std::error_code> ActorContext::SpawnNamed(
    std::shared_ptr<Props> props,
    const std::string& id) {
    auto full_id = self_->id + "/" + id;
    auto [pid, err] = props->Spawn(actor_system_, full_id, shared_from_this());
    if (!err) {
        EnsureExtras()->children_.push_back(pid);
    }
    return {pid, err};
}

void ActorContext::Stop(std::shared_ptr<PID> pid) {
    if (pid) {
        auto process = pid->Ref(actor_system_);
        if (process) {
            process->Stop(pid);
        }
    }
}

std::shared_ptr<Future> ActorContext::StopFuture(std::shared_ptr<PID> pid) {
    auto future = NewFuture(actor_system_, std::chrono::milliseconds(10000));
    auto watch_msg = std::make_shared<protoactor::Watch>(future->GetPID());
    pid->SendSystemMessage(actor_system_, watch_msg);
    Stop(pid);
    return future;
}

void ActorContext::Poison(std::shared_ptr<PID> pid) {
    if (pid) {
        auto poison = std::make_shared<PoisonPill>();
        pid->SendUserMessage(actor_system_, poison);
    }
}

std::shared_ptr<Future> ActorContext::PoisonFuture(std::shared_ptr<PID> pid) {
    auto future = NewFuture(actor_system_, std::chrono::milliseconds(10000));
    auto watch_msg = std::make_shared<protoactor::Watch>(future->GetPID());
    pid->SendSystemMessage(actor_system_, watch_msg);
    Poison(pid);
    return future;
}

void ActorContext::InvokeUserMessage(std::shared_ptr<void> message) {
    if (state_.load(std::memory_order_acquire) == STATE_STOPPED) {
        return;
    }
    
    // Simplified - would handle receive timeout in real implementation
    ProcessMessage(message);
}

void ActorContext::ProcessMessage(std::shared_ptr<void> message) {
    message_or_envelope_ = message;
    DefaultReceive();
    message_or_envelope_ = nullptr;
}

void ActorContext::DefaultReceive() {
    auto msg = Message();

    // Note: PoisonPill handling is done in InvokeSystemMessage
    // We can't safely cast from void* with multiple inheritance

    // Use receiver middleware chain if available
    if (props_ && props_->receiver_middleware_chain_) {
        auto envelope = WrapEnvelope(message_or_envelope_);
        props_->receiver_middleware_chain_(shared_from_this(), envelope);
    } else {
        // Forward to actor directly
        if (actor_) {
            actor_->Receive(shared_from_this());
        }
    }
}

void ActorContext::InvokeSystemMessage(std::shared_ptr<void> message) {
    if (!message) {
        return;
    }

    // Unwrap envelope if present (for messages like Started that need to be
    // delivered to the user actor but are sent through the system mailbox)
    std::shared_ptr<void> actual_message = message;
    auto [header, msg, sender] = UnwrapEnvelope(message);
    if (msg) {
        actual_message = msg;  // Unwrapped message for type checking
    }

    // Try to identify message type by attempting casts
    // Note: This is unsafe but necessary for now
    // In a full implementation, we would use type information or RTTI

    // First, try to cast to SystemMessage
    auto sys_msg = std::static_pointer_cast<protoactor::SystemMessage>(actual_message);
    if (!sys_msg) {
        return; // Not a system message
    }

    // Try Started
    auto started = std::dynamic_pointer_cast<protoactor::Started>(sys_msg);
    if (started) {
        // Started is an AutoReceiveMessage, so process it as a user message
        // Use the original message (envelope) so Message() can properly unwrap it
        ProcessMessage(message);
        return;
    }

    // Try Watch
    auto watch = std::dynamic_pointer_cast<protoactor::Watch>(sys_msg);
    if (watch && watch->watcher) {  // Check for valid Watch structure
        HandleWatch(watch);
        return;
    }

    // Try Unwatch
    auto unwatch = std::dynamic_pointer_cast<protoactor::Unwatch>(sys_msg);
    if (unwatch && unwatch->watcher) {
        HandleUnwatch(unwatch);
        return;
    }

    // Try Stop
    auto stop = std::dynamic_pointer_cast<protoactor::Stop>(sys_msg);
    if (stop) {
        HandleStop();
        return;
    }

    // Try Terminated
    auto terminated = std::dynamic_pointer_cast<protoactor::Terminated>(sys_msg);
    if (terminated && terminated->who) {
        HandleTerminated(terminated);
        return;
    }

    // Try Failure
    auto failure = std::dynamic_pointer_cast<protoactor::Failure>(sys_msg);
    if (failure && failure->who) {
        HandleFailure(failure);
        return;
    }

    // Try Restart
    auto restart = std::dynamic_pointer_cast<protoactor::Restart>(sys_msg);
    if (restart) {
        HandleRestart();
        return;
    }

    // Try Continuation
    auto continuation = std::dynamic_pointer_cast<protoactor::Continuation>(sys_msg);
    if (continuation && continuation->continuation) {
        HandleContinuation(continuation);
        return;
    }
}

void ActorContext::HandleWatch(std::shared_ptr<protoactor::Watch> msg) {
    if (state_.load(std::memory_order_acquire) >= STATE_STOPPING) {
        auto terminated = std::make_shared<protoactor::Terminated>(self_, protoactor::Terminated::Reason::Stopped);
        if (msg->watcher) {
            msg->watcher->SendSystemMessage(actor_system_, terminated);
        }
    } else {
        EnsureExtras()->watchers_.push_back(msg->watcher);
    }
}

void ActorContext::HandleUnwatch(std::shared_ptr<protoactor::Unwatch> msg) {
    if (extras_) {
        auto& watchers = extras_->watchers_;
        watchers.erase(
            std::remove(watchers.begin(), watchers.end(), msg->watcher),
            watchers.end());
    }
}

void ActorContext::HandleStop() {
    state_.store(STATE_STOPPING, std::memory_order_release);
    // Send Stopping message
    // Stop children
    // Send Stopped message
    state_.store(STATE_STOPPED, std::memory_order_release);
}

void ActorContext::HandleTerminated(std::shared_ptr<protoactor::Terminated> msg) {
    // Remove from children
    if (extras_) {
        auto& children = extras_->children_;
        children.erase(
            std::remove(children.begin(), children.end(), msg->who),
            children.end());
    }
    // Forward to actor as user message
    InvokeUserMessage(msg);
}

void ActorContext::HandleFailure(std::shared_ptr<protoactor::Failure> msg) {
    // Escalate to parent
    if (parent_) {
        parent_->SendSystemMessage(actor_system_, msg);
    }
}

void ActorContext::HandleRestart() {
    state_.store(STATE_RESTARTING, std::memory_order_release);
    // Send Restarting message
    InvokeUserMessage(std::make_shared<Restarting>());
    
    // Restart logic
    IncarnateActor();
    
    // Send Started message
    InvokeUserMessage(std::make_shared<Started>());
    
    // Restore stashed messages
    if (extras_ && !extras_->stash_.empty()) {
        while (!extras_->stash_.empty()) {
            auto msg = extras_->stash_.top();
            extras_->stash_.pop();
            InvokeUserMessage(msg);
        }
    }
    
    state_.store(STATE_ALIVE, std::memory_order_release);
}

void ActorContext::HandleContinuation(std::shared_ptr<Continuation> msg) {
    if (!msg || !msg->continuation) {
        return;
    }
    
    // Restore message context
    message_or_envelope_ = msg->message;
    
    // Get future result (would be passed in continuation message in full implementation)
    // For now, call continuation with empty result
    std::shared_ptr<void> result = nullptr;
    std::error_code err;
    
    // In full implementation, the continuation message would contain the future result
    msg->continuation(result, err);
    
    // Clear message context
    message_or_envelope_ = nullptr;
}

void ActorContext::EscalateFailure(std::shared_ptr<void> reason, std::shared_ptr<void> message) {
    if (parent_) {
        auto failure = std::make_shared<protoactor::Failure>(self_, reason, nullptr, message);
        parent_->SendSystemMessage(actor_system_, failure);
    }
}

void ActorContext::SetSelf(std::shared_ptr<PID> self) {
    self_ = self;
}

void ActorContext::ReceiveTimeoutHandler() {
    if (receive_timeout_.count() > 0) {
        CancelReceiveTimeout();
        auto timeout_msg = std::make_shared<protoactor::ReceiveTimeout>();
        Send(self_, timeout_msg);
    }
}

} // namespace protoactor
