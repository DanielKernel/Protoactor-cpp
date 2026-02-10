#include "protoactor/props.h"
#include "protoactor/actor_system.h"
#include "protoactor/context.h"
#include "protoactor/actor_context.h"
#include "protoactor/mailbox.h"
#include "protoactor/dispatcher.h"
#include "protoactor/supervision.h"
#include "protoactor/messages.h"
#include "protoactor/actor_process.h"
#include "protoactor/process_registry.h"
#include "protoactor/middleware_chain.h"
#include <stdexcept>

namespace protoactor {

// Default spawner implementation
static SpawnFunc defaultSpawner = [](std::shared_ptr<ActorSystem> actor_system,
                                      const std::string& id,
                                      std::shared_ptr<Props> props,
                                      std::shared_ptr<Context> parent_context) -> std::pair<std::shared_ptr<PID>, std::error_code> {
    // Create actor context
    auto parent_pid = parent_context ? parent_context->Self() : nullptr;
    auto ctx = ActorContext::New(actor_system, props, parent_pid);
    
    // Create mailbox
    auto mailbox = props->ProduceMailbox();
    
    // Create actor process
    auto process = ActorProcess::New(mailbox);
    
    // Register process
    auto [pid, added] = actor_system->GetProcessRegistry()->Add(process, id);
    if (!added) {
        return {pid, std::make_error_code(std::errc::file_exists)};
    }
    
    ctx->SetSelf(pid);
    
    // Initialize actor (call onInit callbacks)
    // Simplified for now
    
    // Register handlers
    std::shared_ptr<void> invoker = ctx;
    mailbox->RegisterHandlers(invoker, props->GetDispatcher());
    
    // Start mailbox
    mailbox->Start();
    
    // Send Started message
    // Note: PostSystemMessage will schedule processing, which may be synchronous
    // For async dispatcher, this should be fine
    // For sync dispatcher, we need to ensure mailbox is ready
    auto started = std::make_shared<Started>();
    mailbox->PostSystemMessage(started);
    
    // For async dispatcher, the message will be processed in a separate thread
    // For sync dispatcher, we need to ensure the actor is ready before processing
    // The mailbox will handle scheduling
    
    return {pid, std::error_code()};
};

// Default values
static std::shared_ptr<Dispatcher> defaultDispatcher = NewDefaultDispatcher(300);
static MailboxProducer defaultMailboxProducer = Unbounded();
static std::shared_ptr<SupervisorStrategy> defaultSupervisionStrategy = DefaultSupervisorStrategy();

// Props implementation
std::shared_ptr<Props> Props::FromProducer(Producer producer) {
    auto props = std::make_shared<Props>();
    props->producer_ = [producer](std::shared_ptr<ActorSystem>) -> std::shared_ptr<Actor> {
        return producer();
    };
    return props;
}

std::shared_ptr<Props> Props::FromProducerWithActorSystem(ProducerWithActorSystem producer) {
    auto props = std::make_shared<Props>();
    props->producer_ = producer;
    return props;
}

std::shared_ptr<Props> Props::FromFunc(std::function<void(std::shared_ptr<Context>)> receive) {
    class FuncActor : public Actor {
    public:
        explicit FuncActor(std::function<void(std::shared_ptr<Context>)> f) : func_(f) {}
        void Receive(std::shared_ptr<Context> context) override {
            func_(context);
        }
    private:
        std::function<void(std::shared_ptr<Context>)> func_;
    };
    
    return FromProducer([receive]() -> std::shared_ptr<Actor> {
        return std::make_shared<FuncActor>(receive);
    });
}

std::shared_ptr<Props> Props::Configure(std::vector<std::function<void(std::shared_ptr<Props>)>> opts) {
    for (auto& opt : opts) {
        opt(shared_from_this());
    }
    return shared_from_this();
}

SpawnFunc Props::GetSpawner() const {
    return spawner_ ? spawner_ : defaultSpawner;
}

std::shared_ptr<Dispatcher> Props::GetDispatcher() const {
    return dispatcher_ ? dispatcher_ : defaultDispatcher;
}

std::shared_ptr<SupervisorStrategy> Props::GetSupervisor() const {
    return supervision_strategy_ ? supervision_strategy_ : defaultSupervisionStrategy;
}

std::shared_ptr<Mailbox> Props::ProduceMailbox() const {
    if (mailbox_producer_) {
        return mailbox_producer_();
    }
    return defaultMailboxProducer();
}

ProducerWithActorSystem Props::GetProducer() const {
    return producer_;
}

std::shared_ptr<Props> Props::WithSpawner(SpawnFunc spawner) {
    spawner_ = spawner;
    return shared_from_this();
}

std::shared_ptr<Props> Props::WithDispatcher(std::shared_ptr<Dispatcher> dispatcher) {
    dispatcher_ = dispatcher;
    return shared_from_this();
}

std::shared_ptr<Props> Props::WithSupervisor(std::shared_ptr<SupervisorStrategy> supervisor) {
    supervision_strategy_ = supervisor;
    return shared_from_this();
}

std::shared_ptr<Props> Props::WithMailboxProducer(std::function<std::shared_ptr<Mailbox>()> producer) {
    mailbox_producer_ = producer;
    return shared_from_this();
}

std::shared_ptr<Props> Props::WithReceiverMiddleware(std::vector<ReceiverMiddleware> middleware) {
    receiver_middleware_ = middleware;
    // Build receiver middleware chain
    auto lastReceiver = [](std::shared_ptr<Context> ctx, std::shared_ptr<MessageEnvelope> envelope) {
        // Default receiver: invoke actor's Receive
        if (auto actor_ctx = std::dynamic_pointer_cast<ActorContext>(ctx)) {
            auto actor = actor_ctx->GetActor();
            if (actor) {
                actor->Receive(ctx);
            }
        }
    };
    receiver_middleware_chain_ = MakeReceiverMiddlewareChain(middleware, lastReceiver);
    return shared_from_this();
}

std::shared_ptr<Props> Props::WithSenderMiddleware(std::vector<SenderMiddleware> middleware) {
    sender_middleware_ = middleware;
    // Build sender middleware chain
    auto lastSender = [](std::shared_ptr<Context> ctx, std::shared_ptr<PID> target, std::shared_ptr<MessageEnvelope> envelope) {
        // Default sender: send message directly
        if (target) {
            target->SendUserMessage(ctx->GetActorSystem(), envelope);
        }
    };
    sender_middleware_chain_ = MakeSenderMiddlewareChain(middleware, lastSender);
    return shared_from_this();
}

std::shared_ptr<Props> Props::WithSpawnMiddleware(std::vector<SpawnMiddleware> middleware) {
    spawn_middleware_ = middleware;
    // Build spawn middleware chain
    auto lastSpawn = GetSpawner();
    spawn_middleware_chain_ = MakeSpawnMiddlewareChain(middleware, lastSpawn);
    return shared_from_this();
}

std::shared_ptr<Props> Props::WithContextDecorator(std::vector<ContextDecorator> decorators) {
    context_decorator_ = decorators;
    // Build context decorator chain
    auto lastDecorator = [](std::shared_ptr<Context> ctx) -> std::shared_ptr<Context> {
        return ctx; // Default: return context as-is
    };
    context_decorator_chain_ = MakeContextDecoratorChain(decorators, lastDecorator);
    return shared_from_this();
}

std::shared_ptr<Props> Props::WithOnInit(std::vector<std::function<void(std::shared_ptr<Context>)>> init_funcs) {
    on_init_ = init_funcs;
    return shared_from_this();
}

std::pair<std::shared_ptr<PID>, std::error_code> Props::Spawn(
    std::shared_ptr<ActorSystem> actor_system,
    const std::string& name,
    std::shared_ptr<Context> parent_context) {
    // Use spawn middleware chain if available, otherwise use default spawner
    auto spawner = spawn_middleware_chain_ ? spawn_middleware_chain_ : GetSpawner();
    return spawner(actor_system, name, shared_from_this(), parent_context);
}

} // namespace protoactor
