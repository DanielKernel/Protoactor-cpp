/**
 * @file middleware_example.cpp
 * @brief Middleware example demonstrating various middleware patterns
 *
 * This example shows how to use:
 * 1. ReceiverMiddleware - intercept incoming messages
 * 2. SenderMiddleware - intercept outgoing messages
 * 3. SpawnMiddleware - intercept actor spawning
 * 4. ContextDecorator - modify actor context
 *
 * Go equivalent:
 *   props := actor.PropsFromProducer(NewActor).
 *       WithReceiverMiddleware(middleware.Logging, middleware.Tracing)
 */

#include "external/actor.h"
#include "external/context.h"
#include "external/actor_system.h"
#include "external/props.h"
#include "external/messages.h"
#include "external/middleware_chain.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <ctime>

// Message types
struct Hello {
    std::string who;
    explicit Hello(const std::string& w) : who(w) {}
};

struct Ping {
    int id;
    explicit Ping(int i) : id(i) {}
};

struct Pong {
    int id;
    explicit Pong(int i) : id(i) {}
};

// ============================================================================
// Logging Middleware
// ============================================================================

/**
 * @brief Create a logging receiver middleware.
 *
 * Logs all incoming messages to the actor.
 */
protoactor::ReceiverMiddleware LoggingReceiverMiddleware() {
    return [](std::function<void(std::shared_ptr<protoactor::Context>, std::shared_ptr<protoactor::MessageEnvelope>)> next) {
        return [next](std::shared_ptr<protoactor::Context> context, std::shared_ptr<protoactor::MessageEnvelope> envelope) {
            auto now = std::chrono::system_clock::now();
            auto now_time = std::chrono::system_clock::to_time_t(now);

            std::cout << "[LOG] " << std::put_time(std::localtime(&now_time), "%H:%M:%S")
                      << " - Actor " << context->Self()->Address()
                      << " received message" << std::endl;

            // Call next middleware or final receiver
            next(context, envelope);
        };
    };
}

/**
 * @brief Create a logging sender middleware.
 *
 * Logs all outgoing messages from the actor.
 */
protoactor::SenderMiddleware LoggingSenderMiddleware() {
    return [](std::function<void(std::shared_ptr<protoactor::Context>, std::shared_ptr<protoactor::PID>, std::shared_ptr<protoactor::MessageEnvelope>)> next) {
        return [next](std::shared_ptr<protoactor::Context> context, std::shared_ptr<protoactor::PID> target, std::shared_ptr<protoactor::MessageEnvelope> envelope) {
            auto now = std::chrono::system_clock::now();
            auto now_time = std::chrono::system_clock::to_time_t(now);

            std::cout << "[LOG] " << std::put_time(std::localtime(&now_time), "%H:%M:%S")
                      << " - Actor " << context->Self()->Address()
                      << " sending message to " << target->Address() << std::endl;

            next(context, target, envelope);
        };
    };
}

// ============================================================================
// Metrics Middleware
// ============================================================================

/**
 * @brief Metrics tracking middleware.
 *
 * Counts messages received by the actor.
 */
class MetricsMiddleware {
public:
    static protoactor::ReceiverMiddleware Create() {
        return [](std::function<void(std::shared_ptr<protoactor::Context>, std::shared_ptr<protoactor::MessageEnvelope>)> next) {
            return [next](std::shared_ptr<protoactor::Context> context, std::shared_ptr<protoactor::MessageEnvelope> envelope) {
                // Increment message counter (in real app, use atomic or thread-safe counter)
                static int message_count = 0;
                message_count++;

                std::cout << "[METRICS] Total messages processed: " << message_count << std::endl;

                next(context, envelope);
            };
        };
    }
};

// ============================================================================
// Tracing Middleware
// ============================================================================

/**
 * @brief Tracing middleware for debugging.
 *
 * Tracks message flow through the system.
 */
protoactor::ReceiverMiddleware TracingMiddleware() {
    return [](std::function<void(std::shared_ptr<protoactor::Context>, std::shared_ptr<protoactor::MessageEnvelope>)> next) {
        return [next](std::shared_ptr<protoactor::Context> context, std::shared_ptr<protoactor::MessageEnvelope> envelope) {
            // Extract trace ID from message headers if present
            auto headers = envelope->Headers();
            std::string trace_id = "unknown";

            if (headers) {
                // In a real implementation, you would extract trace ID from headers
                // trace_id = headers->Get("trace-id");
            }

            std::cout << "[TRACE] TraceID=" << trace_id
                      << " Actor=" << context->Self()->Address()
                      << " Phase=receive-start" << std::endl;

            next(context, envelope);

            std::cout << "[TRACE] TraceID=" << trace_id
                      << " Actor=" << context->Self()->Address()
                      << " Phase=receive-end" << std::endl;
        };
    };
}

// ============================================================================
// Spawn Middleware
// ============================================================================

/**
 * @brief Spawn middleware for monitoring actor lifecycle.
 *
 * Logs when actors are created.
 */
protoactor::SpawnMiddleware LoggingSpawnMiddleware() {
    return [](protoactor::SpawnFunc next) {
        return [next](std::shared_ptr<protoactor::ActorSystem> system,
                      const std::string& name,
                      std::shared_ptr<protoactor::Props> props,
                      std::shared_ptr<protoactor::Context> context) {
            std::cout << "[SPAWN] Creating actor: " << name << std::endl;

            auto [pid, err] = next(system, name, props, context);

            if (!err) {
                std::cout << "[SPAWN] Actor created successfully: " << pid->Address() << std::endl;
            } else {
                std::cout << "[SPAWN] Failed to create actor: " << err.message() << std::endl;
            }

            return std::make_pair(pid, err);
        };
    };
}

// ============================================================================
// Context Decorator
// ============================================================================

/**
 * @brief Context decorator that adds timing information.
 *
 * This example shows how to wrap context to add custom functionality.
 */
protoactor::ContextDecorator TimingContextDecorator() {
    return [](std::function<std::shared_ptr<protoactor::Context>(std::shared_ptr<protoactor::Context>)> next) {
        return [next](std::shared_ptr<protoactor::Context> ctx) {
            // In a real implementation, you would wrap the context
            // to add timing functionality
            std::cout << "[DECORATOR] Adding timing to context for " << ctx->Self()->Address() << std::endl;
            return next(ctx);
        };
    };
}

// ============================================================================
// Sample Actors
// ============================================================================

/**
 * @brief Simple actor that responds to Ping messages.
 */
class PingActor : public protoactor::Actor {
public:
    void Receive(std::shared_ptr<protoactor::Context> context) override {
        auto msg = context->Message();
        if (!msg) return;

        void* ptr = msg.get();

        if (Hello* hello = static_cast<Hello*>(ptr)) {
            std::cout << "[PingActor] Hello " << hello->who << "!" << std::endl;
        }

        if (Ping* ping = static_cast<Ping*>(ptr)) {
            std::cout << "[PingActor] Received Ping #" << ping->id << std::endl;
            context->Respond(std::make_shared<Pong>(ping->id));
        }
    }
};

/**
 * @brief Actor that sends Pings and receives Pongs.
 */
class ClientActor : public protoactor::Actor {
public:
    void Receive(std::shared_ptr<protoactor::Context> context) override {
        auto msg = context->Message();
        if (!msg) return;

        void* ptr = msg.get();

        if (protoactor::Started* started = dynamic_cast<protoactor::Started*>(ptr)) {
            // On start, send pings to target
            if (target_) {
                for (int i = 1; i <= 3; i++) {
                    context->Send(target_, std::make_shared<Ping>(i));
                }
            }
        }

        if (Pong* pong = static_cast<Pong*>(ptr)) {
            std::cout << "[ClientActor] Received Pong #" << pong->id << std::endl;
        }
    }

    void SetTarget(std::shared_ptr<protoactor::PID> target) { target_ = target; }

private:
    std::shared_ptr<protoactor::PID> target_;
};

// ============================================================================
// Demo Functions
// ============================================================================

void DemoReceiverMiddleware(std::shared_ptr<protoactor::Context> root) {
    std::cout << "\n=== Receiver Middleware Demo ===" << std::endl;
    std::cout << "Receiver middleware intercepts incoming messages.\n" << std::endl;

    // Create props with logging and metrics middleware
    auto props = protoactor::Props::FromProducer([]() {
        return std::make_shared<PingActor>();
    })->WithReceiverMiddleware({
        LoggingReceiverMiddleware(),
        MetricsMiddleware::Create()
    });

    auto pid = root->Spawn(props);

    // Send some messages
    root->Send(pid, std::make_shared<Hello>("Middleware World"));
    root->Send(pid, std::make_shared<Ping>(1));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void DemoSenderMiddleware(std::shared_ptr<protoactor::Context> root) {
    std::cout << "\n=== Sender Middleware Demo ===" << std::endl;
    std::cout << "Sender middleware intercepts outgoing messages.\n" << std::endl;

    // Create target actor
    auto target_props = protoactor::Props::FromProducer([]() {
        return std::make_shared<PingActor>();
    });
    auto target_pid = root->Spawn(target_props);

    // Create client actor with sender middleware
    auto client_props = protoactor::Props::FromProducer([]() {
        return std::make_shared<ClientActor>();
    })->WithSenderMiddleware({
        LoggingSenderMiddleware()
    });

    auto client_pid = root->Spawn(client_props);

    // Set target and start
    // Note: In real implementation, you'd use proper message to set target
}

void DemoCombinedMiddleware(std::shared_ptr<protoactor::Context> root) {
    std::cout << "\n=== Combined Middleware Demo ===" << std::endl;
    std::cout << "Combining multiple middleware for comprehensive logging.\n" << std::endl;

    // Create props with multiple middleware
    auto props = protoactor::Props::FromProducer([]() {
        return std::make_shared<PingActor>();
    })->WithReceiverMiddleware({
        TracingMiddleware(),
        LoggingReceiverMiddleware(),
        MetricsMiddleware::Create()
    })->WithSenderMiddleware({
        LoggingSenderMiddleware()
    });

    auto pid = root->Spawn(props);

    root->Send(pid, std::make_shared<Hello>("Combined"));
    root->Send(pid, std::make_shared<Ping>(42));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void ShowMiddlewareSummary() {
    std::cout << "\n=== Middleware Summary ===" << std::endl;

    std::cout << "\n"
              << "| Middleware Type   | Purpose                          |\n"
              << "|-------------------|----------------------------------|\n"
              << "| Receiver          | Intercept incoming messages      |\n"
              << "| Sender            | Intercept outgoing messages      |\n"
              << "| Spawn             | Monitor actor creation           |\n"
              << "| ContextDecorator  | Extend actor context             |\n"
              << std::endl;

    std::cout << "Common middleware patterns:\n"
              << "  - Logging: Record message flow\n"
              << "  - Metrics: Collect performance data\n"
              << "  - Tracing: Distributed tracing\n"
              << "  - Rate limiting: Control message rate\n"
              << "  - Circuit breaker: Handle failures\n"
              << std::endl;

    std::cout << "Go equivalent:\n"
              << "  // In ProtoActor-Go\n"
              << "  props := actor.PropsFromProducer(NewActor).\n"
              << "      WithReceiverMiddleware(\n"
              << "          middleware.Logging,\n"
              << "          middleware.Tracing,\n"
              << "      )\n"
              << std::endl;
}

int main() {
    std::cout << "=== ProtoActor C++ Middleware Example ===" << std::endl;

    try {
        auto system = protoactor::ActorSystem::New();
        auto root = system->GetRoot();

        // Demonstrate each middleware type
        DemoReceiverMiddleware(root);
        DemoSenderMiddleware(root);
        DemoCombinedMiddleware(root);

        // Show summary
        ShowMiddlewareSummary();

        // Wait for all messages to be processed
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::cout << "\n=== Example Complete ===" << std::endl;

        system->Shutdown();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

/*
 * ============================================================================
 * MIDDLEWARE GUIDE
 * ============================================================================
 *
 * 1. RECEIVER MIDDLEWARE
 * ----------------------
 * Intercept messages BEFORE they reach the actor's Receive method.
 *
 * Use cases:
 *   - Logging incoming messages
 *   - Metrics collection
 *   - Authentication/Authorization
 *   - Message transformation
 *   - Rate limiting
 *
 * C++:
 *   auto props = Props::FromProducer([]() { return std::make_shared<MyActor>(); })
 *       ->WithReceiverMiddleware({ LoggingMiddleware(), MetricsMiddleware() });
 *
 * Go:
 *   props := actor.PropsFromProducer(NewActor).
 *       WithReceiverMiddleware(middleware.Logging)
 *
 *
 * 2. SENDER MIDDLEWARE
 * --------------------
 * Intercept messages BEFORE they are sent to another actor.
 *
 * Use cases:
 *   - Logging outgoing messages
 *   - Adding headers (tracing, correlation IDs)
 *   - Message encryption
 *   - Circuit breaker pattern
 *
 * C++:
 *   auto props = Props::FromProducer([]() { return std::make_shared<MyActor>(); })
 *       ->WithSenderMiddleware({ TracingSenderMiddleware() });
 *
 * Go:
 *   props := actor.PropsFromProducer(NewActor).
 *       WithSenderMiddleware(middleware.Tracing)
 *
 *
 * 3. SPAWN MIDDLEWARE
 * -------------------
 * Intercept actor creation (Spawn calls).
 *
 * Use cases:
 *   - Actor lifecycle monitoring
 *   - Actor pooling
 *   - Dependency injection
 *
 * C++:
 *   auto props = Props::FromProducer([]() { return std::make_shared<MyActor>(); })
 *       ->WithSpawnMiddleware({ LoggingSpawnMiddleware() });
 *
 * Go:
 *   props := actor.PropsFromProducer(NewActor).
 *       WithSpawnMiddleware(middleware.LoggingSpawn)
 *
 *
 * 4. CONTEXT DECORATOR
 * --------------------
 * Wrap/extend the actor context.
 *
 * Use cases:
 *   - Add custom context methods
 *   - Timing/performance measurement
 *   - Request scope data
 *
 * C++:
 *   auto props = Props::FromProducer([]() { return std::make_shared<MyActor>(); })
 *       ->WithContextDecorator({ TimingContextDecorator() });
 *
 * Go:
 *   props := actor.PropsFromProducer(NewActor).
 *       WithContextDecorator(TimingContextDecorator)
 *
 *
 * 5. MIDDLEWARE CHAIN ORDER
 * -------------------------
 * Middleware is executed in order:
 *   - First middleware wraps the next
 *   - Last middleware calls the actual actor
 *
 * Example: [Logging] -> [Metrics] -> [Actor.Receive]
 *
 */
