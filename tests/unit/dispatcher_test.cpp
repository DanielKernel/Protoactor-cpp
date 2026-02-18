/**
 * Unit tests for Dispatcher: default (thread-pool) and synchronized.
 */
#include "protoactor/dispatcher.h"
#include "protoactor/internal/thread_pool.h"
#include "tests/test_common.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <cstdio>

using namespace protoactor;
using namespace protoactor::test;

static bool test_default_dispatcher_schedules_work() {
    auto pool = NewThreadPool(2);
    auto disp = NewDefaultDispatcher(10, pool);
    ASSERT_TRUE(disp->Throughput() == 10);
    std::atomic<int> n(0);
    disp->Schedule([&n]() { n.fetch_add(1, std::memory_order_relaxed); });
    disp->Schedule([&n]() { n.fetch_add(1, std::memory_order_relaxed); });
    while (n.load() < 2) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    ASSERT_EQ(n.load(), 2);
    pool->Shutdown();
    return true;
}

static bool test_sync_dispatcher_runs_inline() {
    auto disp = NewSynchronizedDispatcher(5);
    ASSERT_TRUE(disp->Throughput() == 5);
    std::atomic<int> n(0);
    disp->Schedule([&n]() { n.fetch_add(1, std::memory_order_relaxed); });
    ASSERT_EQ(n.load(), 1);
    disp->Schedule([&n]() { n.fetch_add(1, std::memory_order_relaxed); });
    ASSERT_EQ(n.load(), 2);
    return true;
}

static bool test_default_dispatcher_uses_default_pool_when_null() {
    auto disp = NewDefaultDispatcher(10, nullptr);
    std::atomic<int> n(0);
    disp->Schedule([&n]() { n.fetch_add(1, std::memory_order_relaxed); });
    while (n.load() < 1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    ASSERT_EQ(n.load(), 1);
    return true;
}

static bool test_dispatcher_throughput_value() {
    auto disp1 = NewDefaultDispatcher(100, nullptr);
    auto disp2 = NewSynchronizedDispatcher(200);
    ASSERT_EQ(disp1->Throughput(), 100);
    ASSERT_EQ(disp2->Throughput(), 200);
    return true;
}

int main() {
    std::fprintf(stdout, "Dispatcher tests\n");
    int failed = 0;
#define RUN(name) if (!run_test(#name, name)) ++failed
    RUN(test_default_dispatcher_schedules_work);
    RUN(test_sync_dispatcher_runs_inline);
    RUN(test_default_dispatcher_uses_default_pool_when_null);
    RUN(test_dispatcher_throughput_value);
#undef RUN
    std::fprintf(stdout, "\nTotal: %d failed\n", failed);
    return failed == 0 ? 0 : 1;
}
