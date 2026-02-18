#include "internal/actor/actor_process.h"
#include "internal/mailbox.h"
#include "external/messages.h"

namespace protoactor {

ActorProcess::ActorProcess(std::shared_ptr<Mailbox> mailbox)
    : mailbox_(mailbox), dead_(0) {
}

std::shared_ptr<ActorProcess> ActorProcess::New(std::shared_ptr<Mailbox> mailbox) {
    return std::make_shared<ActorProcess>(mailbox);
}

std::shared_ptr<Mailbox> ActorProcess::GetMailbox() const {
    return mailbox_;
}

bool ActorProcess::IsDead() const {
    return dead_.load(std::memory_order_acquire) == 1;
}

void ActorProcess::SendUserMessage(std::shared_ptr<PID> pid, std::shared_ptr<void> message) {
    mailbox_->PostUserMessage(message);
}

void ActorProcess::SendSystemMessage(std::shared_ptr<PID> pid, std::shared_ptr<void> message) {
    mailbox_->PostSystemMessage(message);
}

void ActorProcess::Stop(std::shared_ptr<PID> pid) {
    dead_.store(1, std::memory_order_release);
    auto stop_msg = std::make_shared<protoactor::Stop>();
    SendSystemMessage(pid, stop_msg);
}

} // namespace protoactor
