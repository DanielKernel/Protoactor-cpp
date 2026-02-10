#include "protoactor/actor.h"
#include "protoactor/context.h"
#include "protoactor/actor_system.h"
#include "protoactor/props.h"
#include "protoactor/supervision.h"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <chrono>

// Message types
struct Hello {
    std::string who;
    Hello(const std::string& w) : who(w) {}
};

// Child actor that may fail
class ChildActor : public protoactor::Actor {
public:
    void Receive(std::shared_ptr<protoactor::Context> context) override {
        auto msg = context->Message();
        if (msg) {
            // Note: In a real implementation, we would use type information
            // For now, we assume msg is actually a std::shared_ptr<Hello>
            // Use aliasing constructor to create shared_ptr<Hello> from shared_ptr<void>
            void* void_ptr = msg.get();
            Hello* raw_ptr = static_cast<Hello*>(void_ptr);
            if (raw_ptr) {
                std::shared_ptr<Hello> hello(msg, raw_ptr);
                std::cout << "Child received: " << hello->who << std::endl;
                // Simulate failure
                throw std::runtime_error("Child actor failure");
            }
        }
    }
};

// Parent actor with supervision
class ParentActor : public protoactor::Actor {
public:
    void Receive(std::shared_ptr<protoactor::Context> context) override {
        auto msg = context->Message();
        if (msg) {
            // Note: In a real implementation, we would use type information
            // For now, we assume msg is actually a std::shared_ptr<Hello>
            // Use aliasing constructor to create shared_ptr<Hello> from shared_ptr<void>
            void* void_ptr = msg.get();
            Hello* raw_ptr = static_cast<Hello*>(void_ptr);
            if (!raw_ptr) {
                return;
            }
            std::shared_ptr<Hello> hello(msg, raw_ptr);
            // Spawn child
            auto child_props = protoactor::Props::FromProducer([]() {
                return std::make_shared<ChildActor>();
            });
            
            // Add supervision strategy
            auto decider = [](std::shared_ptr<void> reason) -> protoactor::Directive {
                std::cout << "Handling failure, reason: " << reason.get() << std::endl;
                return protoactor::Directive::Restart;
            };
            
            auto strategy = protoactor::NewOneForOneStrategy(
                10,
                std::chrono::milliseconds(10000),
                decider);
            
            child_props = child_props->WithSupervisor(strategy);
            
            auto child = context->Spawn(child_props);
            context->Send(child, hello);
        }
    }
};

int main() {
    auto system = protoactor::ActorSystem::New();
    auto root = system->GetRoot();
    
    auto props = protoactor::Props::FromProducer([]() {
        return std::make_shared<ParentActor>();
    });
    
    auto pid = root->Spawn(props);
    
    auto hello = std::make_shared<Hello>("Test");
    root->Send(pid, hello);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    system->Shutdown();
    return 0;
}
