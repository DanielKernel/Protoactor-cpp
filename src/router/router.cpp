#include "protoactor/router/router.h"
#include "protoactor/context.h"
#include "protoactor/pid.h"
#include <random>
#include <algorithm>
#include <cstddef>

namespace protoactor {
namespace router {

// BroadcastRouter implementation
void BroadcastRouter::RouteMessage(std::shared_ptr<void> message) {
    for (auto& routee : routees_) {
        if (sender_) {
            sender_->Send(routee, message);
        }
    }
}

void BroadcastRouter::SetRoutees(const std::vector<std::shared_ptr<PID>>& routees) {
    routees_ = routees;
}

std::vector<std::shared_ptr<PID>> BroadcastRouter::GetRoutees() const {
    return routees_;
}

void BroadcastRouter::SetSender(SenderContext sender) {
    sender_ = sender;
}

// RoundRobinRouter implementation
RoundRobinRouter::RoundRobinRouter()
    : current_index_(0) {
}

void RoundRobinRouter::RouteMessage(std::shared_ptr<void> message) {
    if (routees_.empty()) {
        return;
    }
    
    int index = current_index_.fetch_add(1, std::memory_order_relaxed);
    index = index % static_cast<int>(routees_.size());
    
    if (index < 0) {
        index += routees_.size();
    }
    
    if (sender_ && index < static_cast<int>(routees_.size())) {
        sender_->Send(routees_[index], message);
    }
}

void RoundRobinRouter::SetRoutees(const std::vector<std::shared_ptr<PID>>& routees) {
    routees_ = routees;
    current_index_.store(0, std::memory_order_relaxed);
}

std::vector<std::shared_ptr<PID>> RoundRobinRouter::GetRoutees() const {
    return routees_;
}

void RoundRobinRouter::SetSender(SenderContext sender) {
    sender_ = sender;
}

// RandomRouter implementation
void RandomRouter::RouteMessage(std::shared_ptr<void> message) {
    if (routees_.empty()) {
        return;
    }
    
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, static_cast<int>(routees_.size()) - 1);
    
    int index = dis(gen);
    if (!sender_) {
        return;
    }
    if (index < static_cast<int>(routees_.size())) {
        sender_->Send(routees_[index], message);
    }
}

void RandomRouter::SetRoutees(const std::vector<std::shared_ptr<PID>>& routees) {
    routees_ = routees;
}

std::vector<std::shared_ptr<PID>> RandomRouter::GetRoutees() const {
    return routees_;
}

void RandomRouter::SetSender(SenderContext sender) {
    sender_ = sender;
}

// ConsistentHashRouter implementation
void ConsistentHashRouter::RouteMessage(std::shared_ptr<void> message) {
    if (routees_.empty()) {
        return;
    }
    
    // Simplified consistent hashing - would use proper hash ring in production
    // For now, use a simple hash of the message pointer
    size_t hash = std::hash<std::shared_ptr<void>>()(message);
    int index = static_cast<int>(hash % routees_.size());
    
    if (!sender_) {
        return;
    }
    if (index < static_cast<int>(routees_.size())) {
        sender_->Send(routees_[index], message);
    }
}

void ConsistentHashRouter::SetRoutees(const std::vector<std::shared_ptr<PID>>& routees) {
    routees_ = routees;
}

std::vector<std::shared_ptr<PID>> ConsistentHashRouter::GetRoutees() const {
    return routees_;
}

void ConsistentHashRouter::SetSender(SenderContext sender) {
    sender_ = sender;
}

} // namespace router
} // namespace protoactor
