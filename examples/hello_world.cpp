#include "protoactor/actor.h"
#include "protoactor/context.h"
#include "protoactor/actor_system.h"
#include "protoactor/props.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

// Hello message
struct Hello {
    std::string who;
    
    Hello(const std::string& w) : who(w) {}
};

// HelloActor implementation
class HelloActor : public protoactor::Actor {
public:
    void Receive(std::shared_ptr<protoactor::Context> context) override {
        // Try to cast message to Hello
        auto msg = context->Message();
        if (msg) {
            // Note: In a real implementation, we would use type information
            // For now, we assume msg is actually a std::shared_ptr<Hello>
            // This is unsafe but necessary for the example to work
            // The message was sent as std::shared_ptr<Hello>, so we can safely cast
            // Use aliasing constructor to create shared_ptr<Hello> from shared_ptr<void>
            void* void_ptr = msg.get();
            Hello* raw_ptr = static_cast<Hello*>(void_ptr);
            if (raw_ptr) {
                std::shared_ptr<Hello> hello(msg, raw_ptr);
                std::cout << "Hello " << hello->who << std::endl;
            }
        }
    }
};

int main() {
    try {
        std::cout << "Creating actor system..." << std::endl;
        // Create actor system
        auto system = protoactor::ActorSystem::New();
        auto root = system->GetRoot();
        
        std::cout << "Creating props..." << std::endl;
        // Create props for HelloActor
        auto props = protoactor::Props::FromProducer([]() -> std::shared_ptr<protoactor::Actor> {
            return std::make_shared<HelloActor>();
        });
        
        // Use default dispatcher (async) - synchronized dispatcher has issues with Spawn
        // auto dispatcher = protoactor::NewSynchronizedDispatcher(300);
        // props = props->WithDispatcher(dispatcher);
        
        std::cout << "Spawning actor..." << std::endl;
        // Spawn actor
        auto pid = root->Spawn(props);
        if (!pid) {
            std::cerr << "Failed to spawn actor" << std::endl;
            return 1;
        }
        
        std::cout << "Sending message..." << std::endl;
        // Send message
        auto hello = std::make_shared<Hello>("World");
        std::cout << "About to call root->Send..." << std::endl;
        root->Send(pid, hello);
        std::cout << "root->Send returned" << std::endl;
        
        std::cout << "Waiting for message processing..." << std::endl;
        // Wait longer for message processing (async dispatcher uses separate threads)
        // Give threads time to start and process messages
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        
        std::cout << "Shutting down..." << std::endl;
        // Shutdown
        system->Shutdown();
        
        std::cout << "Done!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}
