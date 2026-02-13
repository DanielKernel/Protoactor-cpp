/**
 * Unit tests for PriorityQueue module.
 */
#include "protoactor/queue/priority_queue.h"
#include "tests/test_common.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <memory>
#include <thread>

using namespace protoactor;
using namespace protoactor::test;

static bool test_priority_queue_empty_initially() {
    PriorityQueue q;
    ASSERT_TRUE(q.Empty());
    ASSERT_EQ(q.Size(), 0u);
    return true;
}

static bool test_priority_queue_push_size() {
    PriorityQueue q;
    q.Push(std::make_shared<int>(1));
    ASSERT_TRUE(!q.Empty());
    ASSERT_EQ(q.Size(), 1u);
    q.Push(std::make_shared<int>(2));
    ASSERT_EQ(q.Size(), 2u);
    return true;
}

static bool test_priority_queue_clear() {
    PriorityQueue q;
    q.Push(std::make_shared<int>(1));
    q.Clear();
    ASSERT_TRUE(q.Empty());
    ASSERT_EQ(q.Size(), 0u);
    return true;
}

static bool test_priority_queue_pop_returns_pushed() {
    PriorityQueue q;
    auto msg = std::make_shared<int>(42);
    q.Push(msg);
    std::shared_ptr<void> out;
    std::atomic<bool> done(false);
    std::thread t([&q, &out, &done]() {
        out = q.Pop();
        done.store(true);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    ASSERT_TRUE(done.load());
    t.join();
    ASSERT_TRUE(out != nullptr);
    ASSERT_TRUE(out == msg);
    return true;
}

int main() {
    std::fprintf(stdout, "PriorityQueue unit tests (module:priority_queue)\n");
    int failed = 0;
#define RUN(name) if (!run_test(#name, name)) ++failed
    RUN(test_priority_queue_empty_initially);
    RUN(test_priority_queue_push_size);
    RUN(test_priority_queue_clear);
    RUN(test_priority_queue_pop_returns_pushed);
#undef RUN
    std::fprintf(stdout, "\nTotal: %d failed\n", failed);
    return failed == 0 ? 0 : 1;
}
