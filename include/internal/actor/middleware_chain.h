#ifndef PROTOACTOR_MIDDLEWARE_CHAIN_H
#define PROTOACTOR_MIDDLEWARE_CHAIN_H

#include "external/props.h"
#include "external/messages.h"
#include "external/context.h"
#include "external/pid.h"
#include <functional>
#include <vector>

namespace protoactor {

/**
 * @brief Build receiver middleware chain.
 * @param middleware Middleware list
 * @param lastReceiver Final receiver function
 * @return Chained receiver function
 */
std::function<void(std::shared_ptr<Context>, std::shared_ptr<MessageEnvelope>)>
MakeReceiverMiddlewareChain(
    const std::vector<ReceiverMiddleware>& middleware,
    std::function<void(std::shared_ptr<Context>, std::shared_ptr<MessageEnvelope>)> lastReceiver);

/**
 * @brief Build sender middleware chain.
 * @param middleware Middleware list
 * @param lastSender Final sender function
 * @return Chained sender function
 */
std::function<void(std::shared_ptr<Context>, std::shared_ptr<PID>, std::shared_ptr<MessageEnvelope>)>
MakeSenderMiddlewareChain(
    const std::vector<SenderMiddleware>& middleware,
    std::function<void(std::shared_ptr<Context>, std::shared_ptr<PID>, std::shared_ptr<MessageEnvelope>)> lastSender);

/**
 * @brief Build spawn middleware chain.
 * @param middleware Middleware list
 * @param lastSpawn Final spawn function
 * @return Chained spawn function
 */
SpawnFunc MakeSpawnMiddlewareChain(
    const std::vector<SpawnMiddleware>& middleware,
    SpawnFunc lastSpawn);

/**
 * @brief Build context decorator chain.
 * @param decorators Decorator list
 * @param lastDecorator Final decorator function
 * @return Chained decorator function
 */
std::function<std::shared_ptr<Context>(std::shared_ptr<Context>)>
MakeContextDecoratorChain(
    const std::vector<ContextDecorator>& decorators,
    std::function<std::shared_ptr<Context>(std::shared_ptr<Context>)> lastDecorator);

} // namespace protoactor

#endif // PROTOACTOR_MIDDLEWARE_CHAIN_H
