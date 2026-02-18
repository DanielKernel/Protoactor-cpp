#include "external/props.h"
#include "external/messages.h"
#include <algorithm>

namespace protoactor {

// Build receiver middleware chain
std::function<void(std::shared_ptr<Context>, std::shared_ptr<MessageEnvelope>)>
MakeReceiverMiddlewareChain(
    const std::vector<ReceiverMiddleware>& middleware,
    std::function<void(std::shared_ptr<Context>, std::shared_ptr<MessageEnvelope>)> lastReceiver) {
    
    if (middleware.empty()) {
        return lastReceiver;
    }
    
    // Build chain from last to first
    auto h = middleware[middleware.size() - 1](lastReceiver);
    for (int i = static_cast<int>(middleware.size()) - 2; i >= 0; --i) {
        h = middleware[i](h);
    }
    
    return h;
}

// Build sender middleware chain
std::function<void(std::shared_ptr<Context>, std::shared_ptr<PID>, std::shared_ptr<MessageEnvelope>)>
MakeSenderMiddlewareChain(
    const std::vector<SenderMiddleware>& middleware,
    std::function<void(std::shared_ptr<Context>, std::shared_ptr<PID>, std::shared_ptr<MessageEnvelope>)> lastSender) {
    
    if (middleware.empty()) {
        return lastSender;
    }
    
    // Build chain from last to first
    auto h = middleware[middleware.size() - 1](lastSender);
    for (int i = static_cast<int>(middleware.size()) - 2; i >= 0; --i) {
        h = middleware[i](h);
    }
    
    return h;
}

// Build spawn middleware chain
SpawnFunc MakeSpawnMiddlewareChain(
    const std::vector<SpawnMiddleware>& middleware,
    SpawnFunc lastSpawn) {
    
    if (middleware.empty()) {
        return lastSpawn;
    }
    
    // Build chain from last to first
    auto h = middleware[middleware.size() - 1](lastSpawn);
    for (int i = static_cast<int>(middleware.size()) - 2; i >= 0; --i) {
        h = middleware[i](h);
    }
    
    return h;
}

// Build context decorator chain
std::function<std::shared_ptr<Context>(std::shared_ptr<Context>)>
MakeContextDecoratorChain(
    const std::vector<ContextDecorator>& decorators,
    std::function<std::shared_ptr<Context>(std::shared_ptr<Context>)> lastDecorator) {
    
    if (decorators.empty()) {
        return lastDecorator;
    }
    
    // Build chain from last to first
    auto h = decorators[decorators.size() - 1](lastDecorator);
    for (int i = static_cast<int>(decorators.size()) - 2; i >= 0; --i) {
        h = decorators[i](h);
    }
    
    return h;
}

} // namespace protoactor
