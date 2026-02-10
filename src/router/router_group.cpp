#include "protoactor/router/router_group.h"
#include "protoactor/context.h"
#include "protoactor/props.h"
#include <algorithm>

namespace protoactor {
namespace router {

RouterActorGroup::RouterActorGroup(std::shared_ptr<Context> context)
    : context_(context) {
}

std::shared_ptr<RouterActorGroup> RouterActorGroup::New(
    std::shared_ptr<Context> context,
    std::shared_ptr<Props> routee_props,
    int count) {
    
    auto group = std::make_shared<RouterActorGroup>(context);
    
    for (int i = 0; i < count; ++i) {
        auto pid = context->Spawn(routee_props);
        if (pid) {
            group->routees_.push_back(pid);
        }
    }
    
    return group;
}

std::vector<std::shared_ptr<PID>> RouterActorGroup::GetRoutees() const {
    return routees_;
}

void RouterActorGroup::AddRoutee(std::shared_ptr<PID> pid) {
    if (pid) {
        routees_.push_back(pid);
    }
}

void RouterActorGroup::RemoveRoutee(std::shared_ptr<PID> pid) {
    routees_.erase(
        std::remove_if(routees_.begin(), routees_.end(),
            [pid](const std::shared_ptr<PID>& p) {
                return p && pid && p->id == pid->id;
            }),
        routees_.end());
}

void RouterActorGroup::Stop() {
    for (auto& pid : routees_) {
        if (pid && context_) {
            context_->Stop(pid);
        }
    }
    routees_.clear();
}

RouterActorPool::RouterActorPool(std::shared_ptr<Context> context)
    : context_(context) {
}

std::shared_ptr<RouterActorPool> RouterActorPool::New(
    std::shared_ptr<Context> context,
    std::shared_ptr<Props> routee_props,
    int pool_size) {
    
    auto pool = std::make_shared<RouterActorPool>(context);
    
    for (int i = 0; i < pool_size; ++i) {
        auto pid = context->Spawn(routee_props);
        if (pid) {
            pool->routees_.push_back(pid);
        }
    }
    
    return pool;
}

std::vector<std::shared_ptr<PID>> RouterActorPool::GetRoutees() const {
    return routees_;
}

void RouterActorPool::Stop() {
    for (auto& pid : routees_) {
        if (pid && context_) {
            context_->Stop(pid);
        }
    }
    routees_.clear();
}

} // namespace router
} // namespace protoactor
