/**
 * Unit tests for Props module: FromProducer, WithDispatcher, GetDispatcher.
 */
#include "external/props.h"
#include "external/actor.h"
#include "external/context.h"
#include "external/dispatcher.h"
#include "tests/test_common.h"
#include <cstdio>
#include <memory>

using namespace protoactor;
using namespace protoactor::test;

static bool test_props_from_producer_not_null() {
    auto props = Props::FromProducer([]() -> std::shared_ptr<Actor> {
        return nullptr;
    });
    ASSERT_TRUE(props != nullptr);
    return true;
}

static bool test_props_with_dispatcher_returns_non_null() {
    auto disp = NewSynchronizedDispatcher(10);
    auto props = Props::FromProducer([]() -> std::shared_ptr<Actor> { return nullptr; });
    auto props2 = props->WithDispatcher(disp);
    ASSERT_TRUE(props2 != nullptr);
    ASSERT_TRUE(props2->GetDispatcher() == disp);
    return true;
}

static bool test_props_from_producer_with_actor_returns_non_null() {
    struct DummyActor : public Actor {
        void Receive(std::shared_ptr<Context>) override {}
    };
    auto props = Props::FromProducer([]() -> std::shared_ptr<Actor> {
        return std::make_shared<DummyActor>();
    });
    ASSERT_TRUE(props != nullptr);
    return true;
}

int main() {
    std::fprintf(stdout, "Props unit tests (module:props)\n");
    int failed = 0;
#define RUN(name) if (!run_test(#name, name)) ++failed
    RUN(test_props_from_producer_not_null);
    RUN(test_props_with_dispatcher_returns_non_null);
    RUN(test_props_from_producer_with_actor_returns_non_null);
#undef RUN
    std::fprintf(stdout, "\nTotal: %d failed\n", failed);
    return failed == 0 ? 0 : 1;
}
