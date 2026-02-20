/**
 * Unit tests for Middleware module: ReceiverMiddleware, SenderMiddleware,
 * SpawnMiddleware, and ContextDecorator chains.
 */
#include "external/props.h"
#include "internal/actor/middleware_chain.h"
#include "external/messages.h"
#include "tests/test_common.h"
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace protoactor;
using namespace protoactor::test;

// ============================================================================
// Receiver Middleware Tests
// ============================================================================

static bool test_empty_receiver_middleware_returns_original() {
    std::vector<ReceiverMiddleware> empty;
    int call_count = 0;

    auto original = [&call_count](std::shared_ptr<Context>, std::shared_ptr<MessageEnvelope>) {
        call_count++;
    };

    auto chain = MakeReceiverMiddlewareChain(empty, original);
    chain(nullptr, nullptr);

    ASSERT_EQ(call_count, 1);
    return true;
}

static bool test_single_receiver_middleware_wraps() {
    std::vector<std::string> order;

    auto middleware = [&order](std::function<void(std::shared_ptr<Context>, std::shared_ptr<MessageEnvelope>)> next) {
        return [&order, next](std::shared_ptr<Context> ctx, std::shared_ptr<MessageEnvelope> env) {
            order.push_back("before");
            next(ctx, env);
            order.push_back("after");
        };
    };

    auto original = [&order](std::shared_ptr<Context>, std::shared_ptr<MessageEnvelope>) {
        order.push_back("original");
    };

    auto chain = MakeReceiverMiddlewareChain({middleware}, original);
    chain(nullptr, nullptr);

    ASSERT_EQ(order.size(), 3);
    ASSERT_TRUE(order[0] == "before");
    ASSERT_TRUE(order[1] == "original");
    ASSERT_TRUE(order[2] == "after");
    return true;
}

static bool test_multiple_receiver_middleware_order() {
    std::vector<int> order;

    auto m1 = [&order](auto next) {
        return [&order, next](auto ctx, auto env) {
            order.push_back(1);
            next(ctx, env);
        };
    };

    auto m2 = [&order](auto next) {
        return [&order, next](auto ctx, auto env) {
            order.push_back(2);
            next(ctx, env);
        };
    };

    auto original = [&order](auto, auto) {
        order.push_back(0);
    };

    auto chain = MakeReceiverMiddlewareChain({m1, m2}, original);
    chain(nullptr, nullptr);

    // Middleware should be called in order: m1, m2, original
    ASSERT_EQ(order.size(), 3);
    ASSERT_EQ(order[0], 1);
    ASSERT_EQ(order[1], 2);
    ASSERT_EQ(order[2], 0);
    return true;
}

// ============================================================================
// Sender Middleware Tests
// ============================================================================

static bool test_empty_sender_middleware_returns_original() {
    std::vector<SenderMiddleware> empty;
    int call_count = 0;

    auto original = [&call_count](std::shared_ptr<Context>, std::shared_ptr<PID>, std::shared_ptr<MessageEnvelope>) {
        call_count++;
    };

    auto chain = MakeSenderMiddlewareChain(empty, original);
    chain(nullptr, nullptr, nullptr);

    ASSERT_EQ(call_count, 1);
    return true;
}

static bool test_sender_middleware_can_modify_target() {
    auto modified_target = NewPID("modified", "target");

    auto middleware = [&modified_target](auto next) {
        return [&modified_target, next](auto ctx, auto /* target */, auto env) {
            next(ctx, modified_target, env);
        };
    };

    std::shared_ptr<PID> captured_target;
    auto original = [&captured_target](auto, auto target, auto) {
        captured_target = target;
    };

    auto chain = MakeSenderMiddlewareChain({middleware}, original);
    auto original_target = NewPID("original", "target");
    chain(nullptr, original_target, nullptr);

    ASSERT_TRUE(captured_target != nullptr);
    ASSERT_TRUE(captured_target->address == modified_target->address);
    return true;
}

// ============================================================================
// Spawn Middleware Tests
// ============================================================================

static bool test_empty_spawn_middleware_returns_original() {
    std::vector<SpawnMiddleware> empty;
    int call_count = 0;

    auto original = [&call_count](auto, auto, auto, auto) -> std::pair<std::shared_ptr<PID>, std::error_code> {
        call_count++;
        return {nullptr, {}};
    };

    auto chain = MakeSpawnMiddlewareChain(empty, original);
    chain(nullptr, "test", nullptr, nullptr);

    ASSERT_EQ(call_count, 1);
    return true;
}

static bool test_spawn_middleware_can_intercept_spawn() {
    bool middleware_called = false;
    bool original_called = false;

    auto middleware = [&middleware_called, &original_called](auto next) {
        return [&middleware_called, &original_called, next](auto system, auto name, auto props, auto ctx) {
            middleware_called = true;
            return next(system, name, props, ctx);
        };
    };

    auto original = [&original_called](auto, auto, auto, auto) -> std::pair<std::shared_ptr<PID>, std::error_code> {
        original_called = true;
        return {nullptr, {}};
    };

    auto chain = MakeSpawnMiddlewareChain({middleware}, original);
    chain(nullptr, "test", nullptr, nullptr);

    ASSERT_TRUE(middleware_called);
    ASSERT_TRUE(original_called);
    return true;
}

// ============================================================================
// Context Decorator Tests
// ============================================================================

static bool test_empty_context_decorator_returns_original() {
    std::vector<ContextDecorator> empty;
    int call_count = 0;

    auto original = [&call_count](std::shared_ptr<Context> ctx) -> std::shared_ptr<Context> {
        call_count++;
        return ctx;
    };

    auto chain = MakeContextDecoratorChain(empty, original);
    chain(nullptr);

    ASSERT_EQ(call_count, 1);
    return true;
}

static bool test_context_decorator_chain() {
    std::vector<int> order;

    auto d1 = [&order](auto next) {
        return [&order, next](auto ctx) {
            order.push_back(1);
            return next(ctx);
        };
    };

    auto d2 = [&order](auto next) {
        return [&order, next](auto ctx) {
            order.push_back(2);
            return next(ctx);
        };
    };

    auto original = [&order](auto ctx) -> std::shared_ptr<Context> {
        order.push_back(0);
        return ctx;
    };

    auto chain = MakeContextDecoratorChain({d1, d2}, original);
    chain(nullptr);

    ASSERT_EQ(order.size(), 3);
    ASSERT_EQ(order[0], 1);
    ASSERT_EQ(order[1], 2);
    ASSERT_EQ(order[2], 0);
    return true;
}

// ============================================================================
// Props Middleware Integration Tests
// ============================================================================

static bool test_props_with_receiver_middleware() {
    auto props = Props::FromProducer([]() -> std::shared_ptr<Actor> {
        return nullptr;
    });

    ReceiverMiddleware middleware = [](auto next) {
        return [next](auto ctx, auto env) {
            next(ctx, env);
        };
    };

    auto props_with_middleware = props->WithReceiverMiddleware({middleware});
    ASSERT_TRUE(props_with_middleware != nullptr);
    return true;
}

static bool test_props_with_sender_middleware() {
    auto props = Props::FromProducer([]() -> std::shared_ptr<Actor> {
        return nullptr;
    });

    SenderMiddleware middleware = [](auto next) {
        return [next](auto ctx, auto target, auto env) {
            next(ctx, target, env);
        };
    };

    auto props_with_middleware = props->WithSenderMiddleware({middleware});
    ASSERT_TRUE(props_with_middleware != nullptr);
    return true;
}

static bool test_props_with_spawn_middleware() {
    auto props = Props::FromProducer([]() -> std::shared_ptr<Actor> {
        return nullptr;
    });

    SpawnMiddleware middleware = [](auto next) {
        return [next](auto system, auto name, auto p, auto ctx) {
            return next(system, name, p, ctx);
        };
    };

    auto props_with_middleware = props->WithSpawnMiddleware({middleware});
    ASSERT_TRUE(props_with_middleware != nullptr);
    return true;
}

static bool test_props_with_context_decorator() {
    auto props = Props::FromProducer([]() -> std::shared_ptr<Actor> {
        return nullptr;
    });

    ContextDecorator decorator = [](auto next) {
        return [next](auto ctx) {
            return next(ctx);
        };
    };

    auto props_with_decorator = props->WithContextDecorator({decorator});
    ASSERT_TRUE(props_with_decorator != nullptr);
    return true;
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::fprintf(stdout, "Middleware unit tests (module:middleware)\n");
    int failed = 0;
#define RUN(name) if (!run_test(#name, name)) ++failed

    // Receiver middleware tests
    RUN(test_empty_receiver_middleware_returns_original);
    RUN(test_single_receiver_middleware_wraps);
    RUN(test_multiple_receiver_middleware_order);

    // Sender middleware tests
    RUN(test_empty_sender_middleware_returns_original);
    RUN(test_sender_middleware_can_modify_target);

    // Spawn middleware tests
    RUN(test_empty_spawn_middleware_returns_original);
    RUN(test_spawn_middleware_can_intercept_spawn);

    // Context decorator tests
    RUN(test_empty_context_decorator_returns_original);
    RUN(test_context_decorator_chain);

    // Props integration tests
    RUN(test_props_with_receiver_middleware);
    RUN(test_props_with_sender_middleware);
    RUN(test_props_with_spawn_middleware);
    RUN(test_props_with_context_decorator);

#undef RUN
    std::fprintf(stdout, "\nTotal: %d failed\n", failed);
    return failed == 0 ? 0 : 1;
}
