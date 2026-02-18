#include "protoactor/deadletter.h"
#include "protoactor/actor_system.h"
#include "protoactor/process_registry.h"
#include "protoactor/eventstream.h"
#include "protoactor/messages.h"
#include "protoactor/pid.h"
#include <iostream>

namespace protoactor {

DeadLetterProcess::DeadLetterProcess(std::shared_ptr<ActorSystem> actor_system)
    : actor_system_(actor_system) {
}

std::shared_ptr<DeadLetterProcess> DeadLetterProcess::New(std::shared_ptr<ActorSystem> actor_system) {
    auto dp = std::make_shared<DeadLetterProcess>(actor_system);
    dp->Initialize();
    return dp;
}

void DeadLetterProcess::Initialize() {
    // Register with process registry
    auto self = std::static_pointer_cast<DeadLetterProcess>(shared_from_this());
    actor_system_->GetProcessRegistry()->Add(self, "deadletter");
    
    // Subscribe to dead letter events
    auto event_stream = actor_system_->GetEventStream();
    event_stream->Subscribe([this](std::shared_ptr<void> msg) {
        // Cast to DeadLetterEvent (assuming msg is DeadLetterEvent*)
        auto dead_letter = std::static_pointer_cast<DeadLetterEvent>(msg);
        if (dead_letter) {
            // Send back a response instead of timeout
            if (dead_letter->sender) {
                auto response = std::make_shared<DeadLetterResponse>(dead_letter->pid);
                actor_system_->GetRoot()->Send(dead_letter->sender, response);
            }
            
            // Log dead letter (simplified - no throttling for now)
            std::cout << "[DeadLetter] pid=" << (dead_letter->pid ? dead_letter->pid->id : "null")
                      << " message=" << dead_letter->message.get()
                      << " sender=" << (dead_letter->sender ? dead_letter->sender->id : "null")
                      << std::endl;
        }
    });
    
    // Handle Watch messages for stopped actors
    event_stream->Subscribe([this](std::shared_ptr<void> msg) {
        auto dead_letter = std::static_pointer_cast<DeadLetterEvent>(msg);
        if (dead_letter && dead_letter->message) {
            // Try to cast message to Watch
            // Note: This is unsafe but necessary for now
            // In a full implementation, we would use type information
            auto watch_msg = std::static_pointer_cast<Watch>(dead_letter->message);
            if (watch_msg && watch_msg->watcher) {
                // Send Terminated message back
                auto terminated = std::make_shared<Terminated>(
                    dead_letter->pid,
                    Terminated::Reason::Stopped);
                watch_msg->watcher->SendSystemMessage(actor_system_, terminated);
            }
        }
    });
}

void DeadLetterProcess::SendUserMessage(std::shared_ptr<PID> pid, std::shared_ptr<void> message) {
    auto [header, msg, sender] = UnwrapEnvelope(message);
    
    auto event = std::make_shared<DeadLetterEvent>(pid, msg, sender);
    actor_system_->GetEventStream()->Publish(event);
}

void DeadLetterProcess::SendSystemMessage(std::shared_ptr<PID> pid, std::shared_ptr<void> message) {
    auto event = std::make_shared<DeadLetterEvent>(pid, message, nullptr);
    actor_system_->GetEventStream()->Publish(event);
}

void DeadLetterProcess::Stop(std::shared_ptr<PID> pid) {
    auto stop_msg = std::shared_ptr<protoactor::Stop>(new protoactor::Stop());
    SendSystemMessage(pid, stop_msg);
}

} // namespace protoactor
