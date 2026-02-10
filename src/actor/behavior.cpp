#include "protoactor/behavior.h"
#include "protoactor/log/log.h"
#include <stdexcept>

namespace protoactor {

std::shared_ptr<Behavior> Behavior::New() {
    return std::make_shared<Behavior>();
}

void Behavior::Become(ReceiveFunc receive) {
    Clear();
    Push(std::move(receive));
}

void Behavior::BecomeStacked(ReceiveFunc receive) {
    Push(std::move(receive));
}

void Behavior::UnbecomeStacked() {
    Pop();
}

void Behavior::Receive(std::shared_ptr<Context> context) {
    auto behavior = Peek();
    if (behavior) {
        behavior(context);
    } else {
        // Log error if behavior is empty
        // In production, this would use proper logging
        throw std::runtime_error("empty behavior called");
    }
}

size_t Behavior::Length() const {
    return behaviors_.size();
}

void Behavior::Clear() {
    behaviors_.clear();
}

Behavior::ReceiveFunc Behavior::Peek() const {
    if (behaviors_.empty()) {
        return nullptr;
    }
    return behaviors_.back();
}

void Behavior::Push(ReceiveFunc receive) {
    behaviors_.push_back(std::move(receive));
}

void Behavior::Pop() {
    if (!behaviors_.empty()) {
        behaviors_.pop_back();
    }
}

} // namespace protoactor
