#ifndef PROTOACTOR_PROPS_H
#define PROTOACTOR_PROPS_H

#include "actor.h"
#include "pid.h"
#include "supervision.h"
#include "protoactor/internal/mailbox.h"
#include "dispatcher.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <system_error>

namespace protoactor {

// Forward declarations
class ActorSystem;
class Context;
struct MessageEnvelope;
class Process;
class Props;  // Forward declare Props before using it in SpawnFunc

/**
 * @brief Spawn function type for creating actors.
 */
using SpawnFunc = std::function<std::pair<std::shared_ptr<PID>, std::error_code>(
    std::shared_ptr<ActorSystem>, const std::string&, std::shared_ptr<Props>, std::shared_ptr<Context>)>;

/**
 * @brief Receiver middleware function type.
 */
using ReceiverMiddleware = std::function<std::function<void(std::shared_ptr<Context>, std::shared_ptr<MessageEnvelope>)>(
    std::function<void(std::shared_ptr<Context>, std::shared_ptr<MessageEnvelope>)>)>;

/**
 * @brief Sender middleware function type.
 */
using SenderMiddleware = std::function<std::function<void(std::shared_ptr<Context>, std::shared_ptr<PID>, std::shared_ptr<MessageEnvelope>)>(
    std::function<void(std::shared_ptr<Context>, std::shared_ptr<PID>, std::shared_ptr<MessageEnvelope>)>)>;

/**
 * @brief Context decorator function type.
 */
using ContextDecorator = std::function<std::function<std::shared_ptr<Context>(std::shared_ptr<Context>)>(
    std::function<std::shared_ptr<Context>(std::shared_ptr<Context>)>)>;

/**
 * @brief Spawn middleware function type.
 */
using SpawnMiddleware = std::function<std::function<std::pair<std::shared_ptr<PID>, std::error_code>(
    std::shared_ptr<ActorSystem>, const std::string&, std::shared_ptr<Props>, std::shared_ptr<Context>)>(
    std::function<std::pair<std::shared_ptr<PID>, std::error_code>(
        std::shared_ptr<ActorSystem>, const std::string&, std::shared_ptr<Props>, std::shared_ptr<Context>)>)>;

/**
 * @brief Props represents configuration to define how an actor should be created.
 */
class Props : public std::enable_shared_from_this<Props> {
public:
    /**
     * @brief Create Props from a producer function.
     * @param producer Function that creates an actor instance
     * @return New Props instance
     */
    static std::shared_ptr<Props> FromProducer(Producer producer);
    
    /**
     * @brief Create Props from a producer function with actor system.
     * @param producer Function that creates an actor instance with actor system
     * @return New Props instance
     */
    static std::shared_ptr<Props> FromProducerWithActorSystem(ProducerWithActorSystem producer);
    
    /**
     * @brief Create Props from a receive function.
     * @param receive Receive function
     * @return New Props instance
     */
    static std::shared_ptr<Props> FromFunc(std::function<void(std::shared_ptr<Context>)> receive);
    
    /**
     * @brief Configure props with options.
     * @param opts Configuration options
     * @return Self for chaining
     */
    std::shared_ptr<Props> Configure(std::vector<std::function<void(std::shared_ptr<Props>)>> opts);
    
    // Getters
    SpawnFunc GetSpawner() const;
    std::shared_ptr<Dispatcher> GetDispatcher() const;
    std::shared_ptr<SupervisorStrategy> GetSupervisor() const;
    std::shared_ptr<Mailbox> ProduceMailbox() const;
    ProducerWithActorSystem GetProducer() const;
    
    // Setters (via Configure)
    std::shared_ptr<Props> WithSpawner(SpawnFunc spawner);
    std::shared_ptr<Props> WithDispatcher(std::shared_ptr<Dispatcher> dispatcher);
    std::shared_ptr<Props> WithSupervisor(std::shared_ptr<SupervisorStrategy> supervisor);
    std::shared_ptr<Props> WithMailboxProducer(std::function<std::shared_ptr<Mailbox>()> producer);
    std::shared_ptr<Props> WithReceiverMiddleware(std::vector<ReceiverMiddleware> middleware);
    std::shared_ptr<Props> WithSenderMiddleware(std::vector<SenderMiddleware> middleware);
    std::shared_ptr<Props> WithSpawnMiddleware(std::vector<SpawnMiddleware> middleware);
    std::shared_ptr<Props> WithContextDecorator(std::vector<ContextDecorator> decorators);
    std::shared_ptr<Props> WithOnInit(std::vector<std::function<void(std::shared_ptr<Context>)>> init_funcs);
    
    /**
     * @brief Spawn an actor using these props.
     * @param actor_system The actor system
     * @param name Actor name
     * @param parent_context Parent context
     * @return PID and error code
     */
    std::pair<std::shared_ptr<PID>, std::error_code> Spawn(
        std::shared_ptr<ActorSystem> actor_system,
        const std::string& name,
        std::shared_ptr<Context> parent_context);

private:
    SpawnFunc spawner_;
    ProducerWithActorSystem producer_;
    std::function<std::shared_ptr<Mailbox>()> mailbox_producer_;
    std::shared_ptr<SupervisorStrategy> guardian_strategy_;
    std::shared_ptr<SupervisorStrategy> supervision_strategy_;
    std::shared_ptr<Dispatcher> dispatcher_;
    std::vector<ReceiverMiddleware> receiver_middleware_;
    std::vector<SenderMiddleware> sender_middleware_;
    std::vector<SpawnMiddleware> spawn_middleware_;
    std::function<void(std::shared_ptr<Context>, std::shared_ptr<MessageEnvelope>)> receiver_middleware_chain_;
    std::function<void(std::shared_ptr<Context>, std::shared_ptr<PID>, std::shared_ptr<MessageEnvelope>)> sender_middleware_chain_;
    SpawnFunc spawn_middleware_chain_;
    std::vector<ContextDecorator> context_decorator_;
    std::function<std::shared_ptr<Context>(std::shared_ptr<Context>)> context_decorator_chain_;
    std::vector<std::function<void(std::shared_ptr<Context>)>> on_init_;
    
    // Make accessible to ActorContext
    friend class ActorContext;
    
public:
    Props() = default;
};

} // namespace protoactor

#endif // PROTOACTOR_PROPS_H
