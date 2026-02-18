/**
 * @file router_example.cpp
 * @brief Router example demonstrating different routing strategies
 *
 * This example shows how to use:
 * 1. BroadcastRouter - sends to all routees
 * 2. RoundRobinRouter - distributes in round-robin fashion
 * 3. RandomRouter - random distribution
 * 4. ConsistentHashRouter - hash-based routing
 *
 * Go equivalent:
 *   props := actor.PropsFromProducer(NewWorker).WithRouter(router.NewRoundRobinPool(5))
 */

#include "external/actor.h"
#include "external/context.h"
#include "external/actor_system.h"
#include "external/props.h"
#include "external/router/router.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <sstream>

// Message types
struct Hello {
    std::string who;
    int id;
    explicit Hello(const std::string& w, int i = 0) : who(w), id(i) {}
};

struct GetCount {};

struct Count {
    int value;
    explicit Count(int v) : value(v) {}
};

/**
 * @brief Simple worker actor that counts messages.
 */
class WorkerActor : public protoactor::Actor {
public:
    void Receive(std::shared_ptr<protoactor::Context> context) override {
        auto msg = context->Message();
        if (!msg) return;

        void* ptr = msg.get();

        if (Hello* hello = static_cast<Hello*>(ptr)) {
            count_++;
            std::cout << "[Worker " << worker_id_ << "] Received Hello #" << hello->id
                      << " from " << hello->who
                      << " (total: " << count_ << ")" << std::endl;
            return;
        }

        if (dynamic_cast<GetCount*>(ptr)) {
            context->Respond(std::make_shared<Count>(count_));
            return;
        }
    }

    void SetWorkerId(int id) { worker_id_ = id; }

private:
    int count_ = 0;
    int worker_id_ = 0;
};

/**
 * @brief Helper function to create worker actors.
 */
std::shared_ptr<protoactor::PID> CreateWorker(
    std::shared_ptr<protoactor::Context> context,
    int worker_id) {

    auto props = protoactor::Props::FromProducer([worker_id]() {
        auto worker = std::make_shared<WorkerActor>();
        worker->SetWorkerId(worker_id);
        return worker;
    });

    auto pid = context->Spawn(props);
    return pid;
}

/**
 * @brief Demonstrate broadcast router - sends to all routees.
 */
void DemoBroadcastRouter(std::shared_ptr<protoactor::Context> root) {
    std::cout << "\n=== Broadcast Router ===" << std::endl;
    std::cout << "Broadcast sends the same message to ALL routees.\n" << std::endl;

    // Create routees (workers)
    std::vector<std::shared_ptr<protoactor::PID>> routees;
    for (int i = 0; i < 3; i++) {
        routees.push_back(CreateWorker(root, i + 1));
    }

    // Create broadcast router
    auto router = std::make_shared<protoactor::router::BroadcastRouter>();
    router->SetRoutees(routees);
    router->SetSender(root);

    // Send one message - it goes to ALL routees
    std::cout << "Sending 1 message to broadcast router..." << std::endl;
    router->RouteMessage(std::make_shared<Hello>("Broadcast", 1));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

/**
 * @brief Demonstrate round-robin router - distributes evenly.
 */
void DemoRoundRobinRouter(std::shared_ptr<protoactor::Context> root) {
    std::cout << "\n=== Round-Robin Router ===" << std::endl;
    std::cout << "Round-Robin distributes messages evenly across routees.\n" << std::endl;

    // Create routees
    std::vector<std::shared_ptr<protoactor::PID>> routees;
    for (int i = 0; i < 3; i++) {
        routees.push_back(CreateWorker(root, i + 10));
    }

    // Create round-robin router
    auto router = std::make_shared<protoactor::router::RoundRobinRouter>();
    router->SetRoutees(routees);
    router->SetSender(root);

    // Send multiple messages - distributed round-robin
    std::cout << "Sending 6 messages through round-robin router..." << std::endl;
    for (int i = 1; i <= 6; i++) {
        router->RouteMessage(std::make_shared<Hello>("RoundRobin", i));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

/**
 * @brief Demonstrate random router - random distribution.
 */
void DemoRandomRouter(std::shared_ptr<protoactor::Context> root) {
    std::cout << "\n=== Random Router ===" << std::endl;
    std::cout << "Random router picks a random routee for each message.\n" << std::endl;

    // Create routees
    std::vector<std::shared_ptr<protoactor::PID>> routees;
    for (int i = 0; i < 3; i++) {
        routees.push_back(CreateWorker(root, i + 20));
    }

    // Create random router
    auto router = std::make_shared<protoactor::router::RandomRouter>();
    router->SetRoutees(routees);
    router->SetSender(root);

    // Send messages - randomly distributed
    std::cout << "Sending 6 messages through random router..." << std::endl;
    for (int i = 1; i <= 6; i++) {
        router->RouteMessage(std::make_shared<Hello>("Random", i));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

/**
 * @brief Demonstrate consistent hash router - hash-based routing.
 */
void DemoConsistentHashRouter(std::shared_ptr<protoactor::Context> root) {
    std::cout << "\n=== Consistent Hash Router ===" << std::endl;
    std::cout << "Consistent Hash routes based on message hash (same key = same routee).\n" << std::endl;

    // Create routees
    std::vector<std::shared_ptr<protoactor::PID>> routees;
    for (int i = 0; i < 3; i++) {
        routees.push_back(CreateWorker(root, i + 30));
    }

    // Create consistent hash router
    auto router = std::make_shared<protoactor::router::ConsistentHashRouter>();
    router->SetRoutees(routees);
    router->SetSender(root);

    // Send messages - same key goes to same routee
    std::cout << "Sending messages with different keys..." << std::endl;
    for (int i = 1; i <= 3; i++) {
        router->RouteMessage(std::make_shared<Hello>("User-" + std::to_string(i), i));
    }

    // Same users again - should go to same routees
    std::cout << "\nSending same users again (should hit same routees)..." << std::endl;
    for (int i = 1; i <= 3; i++) {
        router->RouteMessage(std::make_shared<Hello>("User-" + std::to_string(i), i + 10));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

/**
 * @brief Show router comparison summary.
 */
void ShowRouterSummary() {
    std::cout << "\n=== Router Comparison Summary ===" << std::endl;
    std::cout << "\n"
              << "| Router Type       | Use Case                          |\n"
              << "|-------------------|-----------------------------------|\n"
              << "| Broadcast         | Send to all (notifications)       |\n"
              << "| Round-Robin       | Even distribution (load balance)  |\n"
              << "| Random            | Simple distribution               |\n"
              << "| Consistent Hash   | Sticky routing (sharding)         |\n"
              << std::endl;

    std::cout << "Go equivalent usage:\n"
              << "  // Round-robin pool of 5 workers\n"
              << "  props := actor.PropsFromProducer(NewWorker)\n"
              << "           .WithRouter(router.NewRoundRobinPool(5))\n"
              << "\n"
              << "  // Broadcast group\n"
              << "  props := actor.PropsFromProducer(NewWorker)\n"
              << "           .WithRouter(router.NewBroadcastPool(5))\n"
              << std::endl;
}

int main() {
    std::cout << "=== ProtoActor C++ Router Example ===" << std::endl;

    try {
        auto system = protoactor::ActorSystem::New();
        auto root = system->GetRoot();

        // Demonstrate each router type
        DemoBroadcastRouter(root);
        DemoRoundRobinRouter(root);
        DemoRandomRouter(root);
        DemoConsistentHashRouter(root);

        // Show summary
        ShowRouterSummary();

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
