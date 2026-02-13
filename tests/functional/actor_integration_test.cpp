/**
 * Integration tests for ActorSystem: spawn, send, receive.
 */
#include "protoactor/actor.h"
#include "protoactor/context.h"
#include "protoactor/actor_system.h"
#include "protoactor/props.h"
#include "tests/test_common.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <cstdio>

using namespace protoactor;
using namespace protoactor::test;

struct Ping { std::string value; };

class EchoActor : public protoactor::Actor {
public:
    explicit EchoActor(std::atomic<int>* received) : received_(received) {}
    void Receive(std::shared_ptr<protoactor::Context> ctx) override {
        if (ctx->Message()) {
            received_->fetch_add(1, std::memory_order_relaxed);
        }
    }
private:
    std::atomic<int>* received_;
};

static bool test_spawn_and_send_one_message() {
    std::atomic<int> received(0);
    auto system = ActorSystem::New();
    auto root = system->GetRoot();
    auto props = protoactor::Props::FromProducer([&received]() -> std::shared_ptr<protoactor::Actor> {
        return std::make_shared<EchoActor>(&received);
    });
    auto pid = root->Spawn(props);
    ASSERT_TRUE(pid != nullptr);
    root->Send(pid, std::make_shared<Ping>(Ping{"hello"}));
    for (int i = 0; i < 100; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        if (received.load(std::memory_order_relaxed) >= 1) break;
    }
    ASSERT_GE(received.load(), 1);
    system->Shutdown();
    return true;
}

static bool test_spawn_multiple_actors() {
    std::atomic<int> count(0);
    auto system = ActorSystem::New();
    auto root = system->GetRoot();
    auto props = protoactor::Props::FromProducer([&count]() -> std::shared_ptr<protoactor::Actor> {
        return std::make_shared<EchoActor>(&count);
    });
    std::vector<std::shared_ptr<PID>> pids;
    for (int i = 0; i < 5; ++i) {
        auto pid = root->Spawn(props);
        ASSERT_TRUE(pid != nullptr);
        pids.push_back(pid);
    }
    for (auto& pid : pids) {
        root->Send(pid, std::make_shared<Ping>(Ping{"hi"}));
    }
    for (int i = 0; i < 100; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        if (count.load(std::memory_order_relaxed) >= 5) break;
    }
    ASSERT_GE(count.load(), 5);
    system->Shutdown();
    return true;
}

static bool test_actor_system_new_and_shutdown() {
    auto system = ActorSystem::New();
    ASSERT_TRUE(system != nullptr);
    ASSERT_TRUE(system->GetRoot() != nullptr);
    ASSERT_TRUE(!system->IsStopped());
    system->Shutdown();
    ASSERT_TRUE(system->IsStopped());
    return true;
}

int main() {
    std::fprintf(stdout, "Actor integration tests\n");
    int failed = 0;
#define RUN(name) if (!run_test(#name, name)) ++failed
    RUN(test_actor_system_new_and_shutdown);
    RUN(test_spawn_and_send_one_message);
    RUN(test_spawn_multiple_actors);
#undef RUN
    std::fprintf(stdout, "\nTotal: %d failed\n", failed);
    return failed == 0 ? 0 : 1;
}
