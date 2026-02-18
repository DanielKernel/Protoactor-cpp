/**
 * Unit tests for Queue module (unbounded + MPSC).
 */
#include "protoactor/internal/queue.h"
#include "tests/test_common.h"
#include <cstdio>
#include <memory>
#include <string>

using namespace protoactor;
using namespace protoactor::test;

static bool test_unbounded_queue_empty_initially() {
    auto q = NewUnboundedQueue();
    ASSERT_TRUE(q != nullptr);
    ASSERT_TRUE(q->Empty());
    ASSERT_EQ(q->Size(), 0u);
    return true;
}

static bool test_unbounded_queue_push_pop() {
    auto q = NewUnboundedQueue();
    auto msg = std::make_shared<int>(42);
    q->Push(msg);
    ASSERT_TRUE(!q->Empty());
    ASSERT_EQ(q->Size(), 1u);
    auto out = q->Pop();
    ASSERT_TRUE(out != nullptr);
    ASSERT_TRUE(q->Empty());
    ASSERT_EQ(q->Size(), 0u);
    ASSERT_TRUE(out == msg);
    return true;
}

static bool test_unbounded_queue_pop_empty_returns_null() {
    auto q = NewUnboundedQueue();
    auto out = q->Pop();
    ASSERT_TRUE(out == nullptr);
    return true;
}

static bool test_unbounded_queue_fifo() {
    auto q = NewUnboundedQueue();
    auto a = std::make_shared<int>(1);
    auto b = std::make_shared<int>(2);
    q->Push(a);
    q->Push(b);
    ASSERT_TRUE(q->Pop().get() == a.get());
    ASSERT_TRUE(q->Pop().get() == b.get());
    ASSERT_TRUE(q->Empty());
    return true;
}

static bool test_mpsc_queue_push_pop() {
    auto q = NewMPSCQueue();
    ASSERT_TRUE(q != nullptr);
    auto msg = std::make_shared<int>(99);
    q->Push(msg);
    auto out = q->Pop();
    ASSERT_TRUE(out != nullptr);
    ASSERT_TRUE(out == msg);
    return true;
}

static bool test_mpsc_queue_pop_empty_returns_null() {
    auto q = NewMPSCQueue();
    auto out = q->Pop();
    ASSERT_TRUE(out == nullptr);
    return true;
}

int main() {
    std::fprintf(stdout, "Queue unit tests (module:queue)\n");
    int failed = 0;
#define RUN(name) if (!run_test(#name, name)) ++failed
    RUN(test_unbounded_queue_empty_initially);
    RUN(test_unbounded_queue_push_pop);
    RUN(test_unbounded_queue_pop_empty_returns_null);
    RUN(test_unbounded_queue_fifo);
    RUN(test_mpsc_queue_push_pop);
    RUN(test_mpsc_queue_pop_empty_returns_null);
#undef RUN
    std::fprintf(stdout, "\nTotal: %d failed\n", failed);
    return failed == 0 ? 0 : 1;
}
