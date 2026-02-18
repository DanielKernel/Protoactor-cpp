/**
 * @file remote_example.cpp
 * @brief Remote communication example demonstrating cross-node messaging
 *
 * This example shows how to:
 * 1. Start a remote actor system
 * 2. Register actor kinds for remote spawning
 * 3. Spawn actors on remote nodes
 * 4. Send messages between nodes
 *
 * Go equivalent:
 *   // Node 1 (Server)
 *   remote.Start("localhost:8090")
 *   remote.Register("hello", actor.PropsFromProducer(func() actor.Actor { return &MyActor{} }))
 *
 *   // Node 2 (Client)
 *   remote.Start("localhost:8091")
 *   spawnResponse, _ := remote.SpawnNamed("localhost:8090", "myactor", "hello", time.Second)
 *   context.Send(spawnResponse.Pid, &messages.Echo{Message: "hello"})
 *
 * NOTE: This example requires gRPC and Protobuf to be enabled.
 * Build with: cmake .. -DENABLE_GRPC=ON -DENABLE_PROTOBUF=ON
 */

#include "external/actor.h"
#include "external/context.h"
#include "external/actor_system.h"
#include "external/props.h"
#include "external/messages.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <string>

// Only compile remote code if gRPC is enabled
#ifdef ENABLE_GRPC
#include "external/remote/remote.h"

// Message types for remote communication
// In a real application, these would be defined in .proto files
struct Echo {
    std::string message;
    std::shared_ptr<protoactor::PID> sender;
};

struct Response {
    std::string value;
};

/**
 * @brief Actor that responds to Echo messages.
 * This actor runs on the server node.
 */
class EchoActor : public protoactor::Actor {
public:
    void Receive(std::shared_ptr<protoactor::Context> context) override {
        auto msg = context->Message();
        if (!msg) return;

        void* ptr = msg.get();

        // Handle Echo message
        if (Echo* echo = static_cast<Echo*>(ptr)) {
            std::cout << "[EchoActor] Received: " << echo->message << std::endl;

            // Send response back to sender
            if (echo->sender) {
                context->Send(echo->sender, std::make_shared<Response>(Response{
                    "Echo: " + echo->message
                }));
            }
        }
    }
};

/**
 * @brief Actor that receives responses.
 * This actor runs on the client node.
 */
class ResponseActor : public protoactor::Actor {
public:
    void Receive(std::shared_ptr<protoactor::Context> context) override {
        auto msg = context->Message();
        if (!msg) return;

        void* ptr = msg.get();

        if (Response* resp = static_cast<Response*>(ptr)) {
            std::cout << "[ResponseActor] Got response: " << resp->value << std::endl;
            response_count_++;
        }
    }

    int GetResponseCount() const { return response_count_; }

private:
    int response_count_ = 0;
};

/**
 * @brief Run as server node.
 * Listens for connections and spawns actors on request.
 */
void RunServer(const std::string& host, int port) {
    std::cout << "=== Starting Server Node ===" << std::endl;
    std::cout << "Address: " << host << ":" << port << std::endl;

    auto system = protoactor::ActorSystem::New();

    // Start remote subsystem
    auto remote = protoactor::remote::Remote::Start(system, host, port);

    // Register actor kind for remote spawning
    auto echo_props = protoactor::Props::FromProducer([]() {
        return std::make_shared<EchoActor>();
    });
    remote->Register("echo", echo_props);

    std::cout << "Server started. Registered 'echo' actor kind." << std::endl;
    std::cout << "Press Enter to shutdown..." << std::endl;
    std::cin.get();

    remote->Shutdown(true);
    system->Shutdown();
}

/**
 * @brief Run as client node.
 * Connects to server and spawns remote actors.
 */
void RunClient(const std::string& server_address) {
    std::cout << "=== Starting Client Node ===" << std::endl;
    std::cout << "Connecting to: " << server_address << std::endl;

    auto system = protoactor::ActorSystem::New();
    auto root = system->GetRoot();

    // Start remote subsystem (client also needs remote for responses)
    auto remote = protoactor::remote::Remote::Start(system, "localhost", 8091);

    // Create local response actor
    auto response_props = protoactor::Props::FromProducer([]() {
        return std::make_shared<ResponseActor>();
    });
    auto response_pid = root->Spawn(response_props);

    // Spawn remote actor on server
    std::cout << "Spawning remote actor on server..." << std::endl;
    auto [remote_pid, err] = remote->SpawnNamed(
        server_address,
        "echo",
        "remote-echo-1",
        std::chrono::seconds(5)
    );

    if (err) {
        std::cerr << "Failed to spawn remote actor: " << err.message() << std::endl;
        remote->Shutdown(true);
        system->Shutdown();
        return;
    }

    std::cout << "Remote actor spawned: " << remote_pid->Address() << std::endl;

    // Send messages to remote actor
    for (int i = 1; i <= 5; i++) {
        auto echo = std::make_shared<Echo>();
        echo->message = "Hello " + std::to_string(i);
        echo->sender = response_pid;

        root->Send(remote_pid, echo);
        std::cout << "Sent message " << i << " to remote actor" << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Wait for responses
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "Client done. Press Enter to shutdown..." << std::endl;
    std::cin.get();

    remote->Shutdown(true);
    system->Shutdown();
}

#endif // ENABLE_GRPC

void PrintUsage(const char* program) {
    std::cout << "ProtoActor C++ Remote Example\n\n"
              << "Usage:\n"
              << "  " << program << " server [host] [port]  - Run as server\n"
              << "  " << program << " client [server_addr] - Run as client\n\n"
              << "Examples:\n"
              << "  # Terminal 1 - Start server\n"
              << "  " << program << " server localhost 8090\n\n"
              << "  # Terminal 2 - Start client\n"
              << "  " << program << " client localhost:8090\n"
              << std::endl;

#ifdef ENABLE_GRPC
    std::cout << "gRPC support: ENABLED" << std::endl;
#else
    std::cout << "gRPC support: DISABLED\n"
              << "Rebuild with -DENABLE_GRPC=ON -DENABLE_PROTOBUF=ON to enable remote features."
              << std::endl;
#endif
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        PrintUsage(argv[0]);
        return 1;
    }

    std::string mode = argv[1];

#ifdef ENABLE_GRPC
    if (mode == "server") {
        std::string host = (argc > 2) ? argv[2] : "localhost";
        int port = (argc > 3) ? std::stoi(argv[3]) : 8090;
        RunServer(host, port);
    } else if (mode == "client") {
        std::string server_addr = (argc > 2) ? argv[2] : "localhost:8090";
        RunClient(server_addr);
    } else {
        PrintUsage(argv[0]);
        return 1;
    }
#else
    std::cerr << "Error: Remote features require gRPC support.\n"
              << "Rebuild with: cmake .. -DENABLE_GRPC=ON -DENABLE_PROTOBUF=ON"
              << std::endl;
    return 1;
#endif

    return 0;
}

/*
 * ============================================================================
 * REMOTE COMMUNICATION GUIDE
 * ============================================================================
 *
 * 1. STARTING REMOTE
 * ------------------
 * C++:
 *   auto remote = protoactor::remote::Remote::Start(system, "localhost", 8090);
 *
 * Go:
 *   remote.Start("localhost:8090")
 *
 *
 * 2. REGISTERING ACTOR KINDS
 * --------------------------
 * C++:
 *   auto props = protoactor::Props::FromProducer([]() { return std::make_shared<MyActor>(); });
 *   remote->Register("mykind", props);
 *
 * Go:
 *   remote.Register("mykind", actor.PropsFromProducer(func() actor.Actor { return &MyActor{} }))
 *
 *
 * 3. SPAWNING REMOTE ACTORS
 * -------------------------
 * C++:
 *   auto [pid, err] = remote->SpawnNamed("localhost:8090", "mykind", "myactor", std::chrono::seconds(5));
 *
 * Go:
 *   spawnResponse, err := remote.SpawnNamed("localhost:8090", "mykind", "myactor", time.Second)
 *   pid := spawnResponse.Pid
 *
 *
 * 4. SENDING MESSAGES TO REMOTE ACTORS
 * ------------------------------------
 * Same as local - ProtoActor handles routing automatically:
 *   context->Send(remote_pid, message);
 *
 *
 * 5. SERIALIZATION
 * ----------------
 * ProtoActor uses Protobuf for serialization. Messages must be serializable.
 * Define messages in .proto files for cross-language communication.
 *
 *
 * 6. ARCHITECTURE
 * ---------------
 *   ┌─────────────────┐         gRPC          ┌─────────────────┐
 *   │   Node 1        │ ◄───────────────────► │   Node 2        │
 *   │   ActorSystem   │                       │   ActorSystem   │
 *   │   ┌───────────┐ │                       │   ┌───────────┐ │
 *   │   │ Actor A   │ │                       │   │ Actor B   │ │
 *   │   └───────────┘ │                       │   └───────────┘ │
 *   └─────────────────┘                       └─────────────────┘
 *
 */
