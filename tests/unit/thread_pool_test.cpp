/**
 * Unit tests for ThreadPool: submit, shutdown, drain, exceptions, metrics.
 */
#include "protoactor/thread_pool.h"
#include "tests/test_common.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <cstdio>
#include <stdexcept>

using namespace protoactor;
using namespace protoactor::test;

static bool test_pool_creates_workers() {
    auto pool = NewThreadPool(4);
    ASSERT_TRUE(pool->NumWorkers() == 4);
    ASSERT_TRUE(pool->PendingCount() == 0);
    ASSERT_TRUE(!pool->IsShutdown());
    pool->Shutdown();
    ASSERT_TRUE(pool->IsShutdown());
    return true;
}

static bool test_submit_executes_tasks() {
    auto pool = NewThreadPool(2);
    std::atomic<int> counter(0);
    for (int i = 0; i < 10; ++i) {
        pool->Submit([&counter]() { counter.fetch_add(1, std::memory_order_relaxed); });
    }
    pool->Shutdown(); // drain then join
    ASSERT_EQ(counter.load(), 10);
    return true;
}

static bool test_shutdown_drains_queue() {
    auto pool = NewThreadPool(2);
    std::atomic<int> done(0);
    for (int i = 0; i < 20; ++i) {
        pool->Submit([&done]() { done.fetch_add(1, std::memory_order_relaxed); });
    }
    pool->Shutdown();
    ASSERT_EQ(done.load(), 20);
    return true;
}

static bool test_shutdown_now_drops_pending() {
    auto pool = NewThreadPool(1);
    std::atomic<int> executed(0);
    for (int i = 0; i < 500; ++i) {
        pool->Submit([&executed]() {
            executed.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    pool->ShutdownNow();
    int n = executed.load();
    ASSERT_GE(n, 1);
    ASSERT_TRUE(n < 500); // some must be dropped (queue was cleared)
    return true;
}

static bool test_submit_after_shutdown_is_noop() {
    auto pool = NewThreadPool(2);
    pool->Shutdown();
    std::atomic<int> runs(0);
    pool->Submit([&runs]() { runs.fetch_add(1, std::memory_order_relaxed); });
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    ASSERT_EQ(runs.load(), 0);
    return true;
}

static bool test_shutdown_idempotent() {
    auto pool = NewThreadPool(2);
    pool->Shutdown();
    pool->Shutdown();
    pool->Shutdown();
    ASSERT_TRUE(pool->IsShutdown());
    return true;
}

static bool test_exception_in_task_does_not_kill_worker() {
    auto pool = NewThreadPool(2);
    std::atomic<int> ok_count(0);
    for (int i = 0; i < 6; ++i) {
        pool->Submit([&ok_count, i]() {
            if (i % 2 == 0) throw std::runtime_error("test");
            ok_count.fetch_add(1, std::memory_order_relaxed);
        });
    }
    pool->Shutdown();
    ASSERT_GE(ok_count.load(), 2);
    ASSERT_GE(3, ok_count.load());
    return true;
}

static bool test_pending_count_approximate() {
    auto pool = NewThreadPool(1);
    for (int i = 0; i < 5; ++i) {
        pool->Submit([]() { std::this_thread::sleep_for(std::chrono::milliseconds(20)); });
    }
    std::size_t pending = pool->PendingCount();
    ASSERT_GE(pending, 0);
    ASSERT_TRUE(pending <= 5);
    pool->Shutdown();
    return true;
}

static bool test_default_pool_exists() {
    auto p = DefaultThreadPool();
    ASSERT_TRUE(p != nullptr);
    ASSERT_TRUE(p->NumWorkers() >= 1);
    return true;
}

int main() {
    std::fprintf(stdout, "ThreadPool tests\n");
    int failed = 0;
#define RUN(name) if (!run_test(#name, name)) ++failed
    RUN(test_pool_creates_workers);
    RUN(test_submit_executes_tasks);
    RUN(test_shutdown_drains_queue);
    RUN(test_shutdown_now_drops_pending);
    RUN(test_submit_after_shutdown_is_noop);
    RUN(test_shutdown_idempotent);
    RUN(test_exception_in_task_does_not_kill_worker);
    RUN(test_pending_count_approximate);
    RUN(test_default_pool_exists);
#undef RUN
    std::fprintf(stdout, "\nTotal: %d failed\n", failed);
    return failed == 0 ? 0 : 1;
}
