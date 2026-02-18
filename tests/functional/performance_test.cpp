/**
 * Performance tests: thread pool throughput, dispatcher throughput, actor message throughput.
 * Run in Release build for meaningful numbers. Output is human-readable report.
 */
#include "internal/thread_pool.h"
#include "external/dispatcher.h"
#include "external/actor.h"
#include "external/context.h"
#include "external/actor_system.h"
#include "external/props.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <memory>
#include <string>
#include <thread>

using namespace protoactor;

static double now_sec() {
    using clock = std::chrono::high_resolution_clock;
    return 1e-9 * static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        clock::now().time_since_epoch()).count());
}

static void bench_thread_pool_throughput() {
    const int num_tasks = 100000;
    const int num_workers = 4;
    auto pool = NewThreadPool(static_cast<std::size_t>(num_workers));
    std::atomic<int> done(0);
    double t0 = now_sec();
    for (int i = 0; i < num_tasks; ++i) {
        pool->Submit([&done]() { done.fetch_add(1, std::memory_order_relaxed); });
    }
    pool->Shutdown();
    double t1 = now_sec();
    int n = done.load();
    double sec = t1 - t0;
    double throughput = (sec > 0 && n > 0) ? (n / sec) : 0;
    std::fprintf(stdout, "[perf] ThreadPool: %d tasks, %d workers, %.3f s => %.0f tasks/s\n",
                 num_tasks, num_workers, sec, throughput);
}

static void bench_dispatcher_throughput() {
    const int num_schedules = 50000;
    auto pool = NewThreadPool(4);
    auto disp = NewDefaultDispatcher(10, pool);
    std::atomic<int> done(0);
    double t0 = now_sec();
    for (int i = 0; i < num_schedules; ++i) {
        disp->Schedule([&done]() { done.fetch_add(1, std::memory_order_relaxed); });
    }
    while (done.load(std::memory_order_relaxed) < num_schedules) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    double t1 = now_sec();
    pool->Shutdown();
    double sec = t1 - t0;
    double throughput = (sec > 0) ? (num_schedules / sec) : 0;
    std::fprintf(stdout, "[perf] Dispatcher (thread-pool): %d schedules => %.0f schedules/s\n",
                 num_schedules, throughput);
}

struct BenchMsg { int id; };

class BenchActor : public Actor {
public:
    explicit BenchActor(std::atomic<int>* count) : count_(count) {}
    void Receive(std::shared_ptr<Context> ctx) override {
        if (ctx->Message())
            count_->fetch_add(1, std::memory_order_relaxed);
    }
private:
    std::atomic<int>* count_;
};

static void bench_actor_message_throughput() {
    const int num_messages = 10000;
    std::atomic<int> received(0);
    auto system = ActorSystem::New();
    auto root = system->GetRoot();
    auto props = Props::FromProducer([&received]() -> std::shared_ptr<Actor> {
        return std::make_shared<BenchActor>(&received);
    });
    auto pid = root->Spawn(props);
    if (!pid) {
        std::fprintf(stderr, "[perf] Actor spawn failed\n");
        system->Shutdown();
        return;
    }
    double t0 = now_sec();
    for (int i = 0; i < num_messages; ++i) {
        root->Send(pid, std::make_shared<BenchMsg>(BenchMsg{i}));
    }
    for (int wait = 0; wait < 500; ++wait) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (received.load(std::memory_order_relaxed) >= num_messages) break;
    }
    double t1 = now_sec();
    system->Shutdown();
    int n = received.load();
    double sec = t1 - t0;
    double throughput = (sec > 0 && n > 0) ? (n / sec) : 0;
    std::fprintf(stdout, "[perf] Actor messages: %d sent, %d received, %.3f s => %.0f msg/s\n",
                 num_messages, n, sec, throughput);
}

int main() {
    std::fprintf(stdout, "=== ProtoActor C++ performance tests ===\n");
    std::fprintf(stdout, "Build: %s\n", (sizeof(void*) == 8 ? "64-bit" : "32-bit"));
    std::fprintf(stdout, "\n");

    bench_thread_pool_throughput();
    bench_dispatcher_throughput();
    bench_actor_message_throughput();

    std::fprintf(stdout, "\nDone.\n");
    return 0;
}
