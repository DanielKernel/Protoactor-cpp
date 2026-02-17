/**
 * @file behavior_example.cpp
 * @brief Behavior management example demonstrating SetBehavior/PushBehavior/PopBehavior
 *
 * This example shows how to implement state machine patterns using Behavior.
 * Similar to Go version's context.SetBehavior() pattern.
 *
 * Go equivalent:
 *   func (state *SetBehaviorActor) Receive(context actor.Context) {
 *       switch msg := context.Message().(type) {
 *       case Hello:
 *           fmt.Printf("Hello %v\n", msg.Who)
 *           context.SetBehavior(state.Other)
 *       }
 *   }
 */

#include "protoactor/actor.h"
#include "protoactor/context.h"
#include "protoactor/actor_system.h"
#include "protoactor/props.h"
#include "protoactor/behavior.h"
#include "protoactor/messages.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

// Message types
struct Hello {
    std::string who;
    explicit Hello(const std::string& w) : who(w) {}
};

struct BecomeUnbecome {};
struct BecomeStacked {};
struct UnbecomeStacked {};

/**
 * @brief Actor demonstrating behavior state machine.
 *
 * This actor shows how to:
 * 1. Change behavior with Become (replace current)
 * 2. Stack behavior with BecomeStacked (push)
 * 3. Revert behavior with UnbecomeStacked (pop)
 */
class BehaviorActor : public protoactor::Actor {
public:
    BehaviorActor() : behavior_(protoactor::Behavior::New()) {}

    void Receive(std::shared_ptr<protoactor::Context> context) override {
        // First, let the current behavior handle the message
        if (behavior_->Length() > 0) {
            behavior_->Receive(context);
            return;
        }

        // Initial behavior - handle messages here
        auto msg = context->Message();
        if (!msg) return;

        // Check for Hello message
        if (TryHandle<Hello>(msg, [](std::shared_ptr<Hello> hello) {
            std::cout << "[Initial] Hello " << hello->who << std::endl;
        })) return;

        // Check for BecomeUnbecome - switch to alternate behavior
        if (TryHandle<BecomeUnbecome>(msg, [this]() {
            std::cout << "[Initial] Changing to Alternate behavior (Become)" << std::endl;
            behavior_->Become([this](std::shared_ptr<protoactor::Context> ctx) {
                AlternateBehavior(ctx);
            });
        })) return;

        // Check for BecomeStacked - push a new behavior
        if (TryHandle<BecomeStacked>(msg, [this]() {
            std::cout << "[Initial] Pushing Stacked behavior (BecomeStacked)" << std::endl;
            behavior_->BecomeStacked([this](std::shared_ptr<protoactor::Context> ctx) {
                StackedBehavior(ctx);
            });
        })) return;
    }

private:
    std::shared_ptr<protoactor::Behavior> behavior_;

    // Helper to check message type
    template<typename T, typename Handler>
    bool TryHandle(std::shared_ptr<void> msg, Handler handler) {
        void* ptr = msg.get();
        if (T* typed = static_cast<T*>(ptr)) {
            std::shared_ptr<T> typed_msg(msg, typed);
            handler(typed_msg);
            return true;
        }
        return false;
    }

    // Alternate behavior - replaces the initial behavior
    void AlternateBehavior(std::shared_ptr<protoactor::Context> context) {
        auto msg = context->Message();
        if (!msg) return;

        if (TryHandle<Hello>(msg, [](std::shared_ptr<Hello> hello) {
            std::cout << "[Alternate] Hello " << hello->who << " (different response!)" << std::endl;
        })) return;

        if (TryHandle<BecomeUnbecome>(msg, [this]() {
            std::cout << "[Alternate] Changing back to Initial behavior" << std::endl;
            behavior_->Become([this](std::shared_ptr<protoactor::Context> ctx) {
                InitialBehavior(ctx);
            });
        })) return;
    }

    // Stacked behavior - pushed on top of current behavior
    void StackedBehavior(std::shared_ptr<protoactor::Context> context) {
        auto msg = context->Message();
        if (!msg) return;

        if (TryHandle<Hello>(msg, [](std::shared_ptr<Hello> hello) {
            std::cout << "[Stacked] Hello " << hello->who << " (stacked behavior!)" << std::endl;
        })) return;

        if (TryHandle<UnbecomeStacked>(msg, [this]() {
            std::cout << "[Stacked] Popping back to previous behavior (UnbecomeStacked)" << std::endl;
            behavior_->UnbecomeStacked();
        })) return;
    }

    // Initial behavior handler (for Become back to initial)
    void InitialBehavior(std::shared_ptr<protoactor::Context> context) {
        auto msg = context->Message();
        if (!msg) return;

        if (TryHandle<Hello>(msg, [](std::shared_ptr<Hello> hello) {
            std::cout << "[Initial] Hello " << hello->who << std::endl;
        })) return;
    }
};

/**
 * @brief Actor demonstrating lifecycle events.
 *
 * Proto.Actor uses messages for lifecycle events instead of OOP method overrides.
 * This is consistent with the Go version.
 */
class LifecycleActor : public protoactor::Actor {
public:
    void Receive(std::shared_ptr<protoactor::Context> context) override {
        auto msg = context->Message();
        if (!msg) return;

        // Lifecycle events - these are system messages
        void* ptr = msg.get();

        // Started - actor is initialized
        if (dynamic_cast<protoactor::Started*>(ptr)) {
            std::cout << "[Lifecycle] Started - actor is ready!" << std::endl;
            return;
        }

        // Stopping - actor is about to stop
        if (dynamic_cast<protoactor::Stopping*>(ptr)) {
            std::cout << "[Lifecycle] Stopping - actor is shutting down..." << std::endl;
            return;
        }

        // Stopped - actor has stopped
        if (dynamic_cast<protoactor::Stopped*>(ptr)) {
            std::cout << "[Lifecycle] Stopped - actor is now stopped." << std::endl;
            return;
        }

        // Restarting - actor is about to restart
        if (dynamic_cast<protoactor::Restarting*>(ptr)) {
            std::cout << "[Lifecycle] Restarting - actor is about to restart..." << std::endl;
            return;
        }

        // Regular messages
        if (Hello* hello = static_cast<Hello*>(ptr)) {
            std::cout << "[Lifecycle] Received Hello: " << hello->who << std::endl;
        }
    }
};

int main() {
    std::cout << "=== ProtoActor C++ Behavior Example ===" << std::endl;
    std::cout << std::endl;

    try {
        auto system = protoactor::ActorSystem::New();
        auto root = system->GetRoot();

        // === Behavior State Machine Example ===
        std::cout << "--- Behavior State Machine ---" << std::endl;

        auto behavior_props = protoactor::Props::FromProducer([]() {
            return std::make_shared<BehaviorActor>();
        });
        auto behavior_pid = root->Spawn(behavior_props);

        // Send message - handled by initial behavior
        root->Send(behavior_pid, std::make_shared<Hello>("World"));

        // Change behavior
        root->Send(behavior_pid, std::make_shared<BecomeUnbecome>());

        // Now handled by alternate behavior
        root->Send(behavior_pid, std::make_shared<Hello>("Again"));

        // Change back to initial
        root->Send(behavior_pid, std::make_shared<BecomeUnbecome>());

        // Handled by initial behavior again
        root->Send(behavior_pid, std::make_shared<Hello>("Back to initial"));

        std::cout << std::endl;
        std::cout << "--- Behavior Stacking ---" << std::endl;

        // Spawn a fresh actor for stacking demo
        auto stacked_pid = root->Spawn(behavior_props);

        // Push stacked behavior
        root->Send(stacked_pid, std::make_shared<BecomeStacked>());

        // Handled by stacked behavior
        root->Send(stacked_pid, std::make_shared<Hello>("Stacked"));

        // Pop back to initial
        root->Send(stacked_pid, std::make_shared<UnbecomeStacked>());

        // Back to initial behavior
        root->Send(stacked_pid, std::make_shared<Hello>("Unstacked"));

        std::cout << std::endl;

        // === Lifecycle Example ===
        std::cout << "--- Lifecycle Events ---" << std::endl;

        auto lifecycle_props = protoactor::Props::FromProducer([]() {
            return std::make_shared<LifecycleActor>();
        });
        auto lifecycle_pid = root->Spawn(lifecycle_props);

        root->Send(lifecycle_pid, std::make_shared<Hello>("Lifecycle Test"));

        // Wait a bit before stopping
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Stop the actor - this will trigger Stopping and Stopped events
        root->Stop(lifecycle_pid);

        // Wait for messages to be processed
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::cout << std::endl;
        std::cout << "=== Example Complete ===" << std::endl;

        system->Shutdown();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
