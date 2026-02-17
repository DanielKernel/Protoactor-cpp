/**
 * @file cluster_example.cpp
 * @brief Cluster example demonstrating distributed actor management
 *
 * This example shows how to:
 * 1. Configure and start a cluster
 * 2. Use member management and Gossip protocol
 * 3. Work with virtual actors (Grains)
 * 4. Use Pub/Sub messaging
 *
 * Go equivalent:
 *   cluster.Start()
 *   memberList := cluster.GetMemberList()
 *   grain := cluster.GetGrain("my-grain", "grain-id")
 *
 * NOTE: This example requires gRPC and Protobuf to be enabled.
 * Build with: cmake .. -DENABLE_GRPC=ON -DENABLE_PROTOBUF=ON
 */

#include "protoactor/actor.h"
#include "protoactor/context.h"
#include "protoactor/actor_system.h"
#include "protoactor/props.h"
#include "protoactor/messages.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <string>

#ifdef ENABLE_GRPC
#include "protoactor/cluster/cluster.h"
#include "protoactor/cluster/member_list.h"
#include "protoactor/cluster/gossiper.h"

// Message types
struct Hello {
    std::string message;
};

/**
 * @brief Grain (virtual actor) implementation.
 *
 * Grains are virtual actors that can be activated on any cluster member.
 * They are identified by kind + identity (like "user-123").
 */
class HelloGrain : public protoactor::Actor {
public:
    void Receive(std::shared_ptr<protoactor::Context> context) override {
        auto msg = context->Message();
        if (!msg) return;

        void* ptr = msg.get();

        if (Hello* hello = static_cast<Hello*>(ptr)) {
            std::cout << "[HelloGrain@" << context->Self()->Address()
                      << "] Received: " << hello->message
                      << " (activation: " << activation_count_ << ")" << std::endl;
            activation_count_++;
        }
    }

private:
    int activation_count_ = 0;
};

/**
 * @brief Actor that monitors cluster topology changes.
 */
class TopologyMonitor : public protoactor::Actor {
public:
    void Receive(std::shared_ptr<protoactor::Context> context) override {
        auto msg = context->Message();
        if (!msg) return;

        // Handle cluster topology events
        // In a real implementation, you would subscribe to topology events
        // via the event stream
        std::cout << "[TopologyMonitor] Topology event received" << std::endl;
    }
};

/**
 * @brief Run a cluster member node.
 */
void RunClusterNode(const std::string& cluster_name,
                    const std::string& host,
                    int port,
                    const std::vector<std::string>& seeds = {}) {
    std::cout << "=== Starting Cluster Node ===" << std::endl;
    std::cout << "Cluster: " << cluster_name << std::endl;
    std::cout << "Address: " << host << ":" << port << std::endl;

    // Create actor system
    auto system = protoactor::ActorSystem::New();

    // Configure cluster
    auto cluster_config = protoactor::cluster::Config::New(cluster_name, host, port);

    // Add seed nodes for discovery (if any)
    // In production, you would use Consul, Kubernetes, etc.

    // Create and start cluster
    auto cluster = protoactor::cluster::Cluster::New(system, cluster_config);

    // Register grain kinds
    auto grain_props = protoactor::Props::FromProducer([]() {
        return std::make_shared<HelloGrain>();
    });
    cluster->GetRemote()->Register("hello-grain", grain_props);

    // Start the cluster
    cluster->Start();
    std::cout << "Cluster started." << std::endl;

    // Get member list
    auto member_list = cluster->GetMemberList();
    std::cout << "Current members: " << member_list->MemberCount() << std::endl;

    // Demonstrate grain usage
    auto root = system->GetRoot();

    // Get a grain by identity (will be activated on some member)
    // auto grain_pid = cluster->Get("hello-grain", "user-123");

    // Send message to grain
    // root->Send(grain_pid, std::make_shared<Hello>("Hello from cluster!"));

    std::cout << "\nCluster node running. Press Enter to shutdown..." << std::endl;
    std::cin.get();

    // Shutdown
    cluster->Shutdown();
    system->Shutdown();
}

/**
 * @brief Demonstrate cluster member management.
 */
void DemoMemberManagement(std::shared_ptr<protoactor::cluster::Cluster> cluster) {
    std::cout << "\n=== Member Management ===" << std::endl;

    auto member_list = cluster->GetMemberList();

    // Get all members
    auto members = member_list->GetMembers();
    std::cout << "Cluster members (" << members.size() << "):" << std::endl;

    for (const auto& member : members) {
        std::cout << "  - " << member->Address()
                  << " (kinds: " << member->Kinds().size() << ")" << std::endl;
    }

    // Check if specific member exists
    // bool alive = member_list->MemberExists("member-id");
}

/**
 * @brief Demonstrate Gossip protocol.
 */
void DemoGossip(std::shared_ptr<protoactor::cluster::Cluster> cluster) {
    std::cout << "\n=== Gossip Protocol ===" << std::endl;

    // Get gossiper
    // auto gossiper = cluster->GetGossiper();

    // Set local state
    // gossiper->SetState("key", value);

    // Get state from cluster
    // auto state = gossiper->GetState("key");
}

#endif // ENABLE_GRPC

void PrintUsage(const char* program) {
    std::cout << "ProtoActor C++ Cluster Example\n\n"
              << "Usage:\n"
              << "  " << program << " node <cluster_name> <host> <port> [seed_nodes...]\n\n"
              << "Examples:\n"
              << "  # Start first node (seed)\n"
              << "  " << program << " node mycluster localhost 8090\n\n"
              << "  # Start second node (joining cluster)\n"
              << "  " << program << " node mycluster localhost 8091 localhost:8090\n"
              << std::endl;

#ifdef ENABLE_GRPC
    std::cout << "gRPC/Cluster support: ENABLED" << std::endl;
#else
    std::cout << "gRPC/Cluster support: DISABLED\n"
              << "Rebuild with -DENABLE_GRPC=ON -DENABLE_PROTOBUF=ON"
              << std::endl;
#endif
}

int main(int argc, char* argv[]) {
    if (argc < 5) {
        PrintUsage(argv[0]);
        return 1;
    }

    std::string mode = argv[1];

#ifdef ENABLE_GRPC
    if (mode == "node") {
        std::string cluster_name = argv[2];
        std::string host = argv[3];
        int port = std::stoi(argv[4]);

        std::vector<std::string> seeds;
        for (int i = 5; i < argc; i++) {
            seeds.push_back(argv[i]);
        }

        RunClusterNode(cluster_name, host, port, seeds);
    } else {
        PrintUsage(argv[0]);
        return 1;
    }
#else
    std::cerr << "Error: Cluster features require gRPC support.\n"
              << "Rebuild with: cmake .. -DENABLE_GRPC=ON -DENABLE_PROTOBUF=ON"
              << std::endl;
    return 1;
#endif

    return 0;
}

/*
 * ============================================================================
 * CLUSTER GUIDE
 * ============================================================================
 *
 * 1. CLUSTER CONCEPTS
 * -------------------
 *
 * - Member: A node in the cluster
 * - Kind: A type of actor/grain that can be activated
 * - Grain (Virtual Actor): Automatically activated on any member
 * - Identity: Unique identifier for a grain instance
 * - Gossip: Protocol for cluster-wide state replication
 *
 *
 * 2. STARTING A CLUSTER
 * ---------------------
 * C++:
 *   auto config = protoactor::cluster::Config::New("mycluster", "localhost", 8090);
 *   auto cluster = protoactor::cluster::Cluster::New(system, config);
 *   cluster->Start();
 *
 * Go:
 *   config := cluster.Configure("mycluster", "localhost", 8090, ...)
 *   c := cluster.New(system, config)
 *   c.Start()
 *
 *
 * 3. VIRTUAL ACTORS (GRAINS)
 * --------------------------
 *
 * Grains are automatically activated:
 * - First message triggers activation on suitable member
 * - Identity determines routing (consistent hash)
 * - Transparent failover if member fails
 *
 * C++:
 *   // Register grain kind
 *   cluster->GetRemote()->Register("my-grain", props);
 *
 *   // Get grain by identity
 *   auto grain = cluster->Get("my-grain", "user-123");
 *
 *   // Send message
 *   context->Send(grain, message);
 *
 * Go:
 *   // Register grain
 *   cluster.RegisterKind("my-grain", props)
 *
 *   // Get grain
 *   grain := cluster.Get("my-grain", "user-123")
 *
 *
 * 4. MEMBER MANAGEMENT
 * --------------------
 *
 * C++:
 *   auto memberList = cluster->GetMemberList();
 *   auto members = memberList->GetMembers();
 *   int count = memberList->MemberCount();
 *
 * Go:
 *   memberList := cluster.GetMemberList()
 *   members := memberList.Members()
 *
 *
 * 5. GOSSIP PROTOCOL
 * ------------------
 *
 * Used for cluster-wide state replication (leader election, config, etc.)
 *
 * C++:
 *   auto gossiper = cluster->GetGossiper();
 *   gossiper->SetState("leader", leaderPid);
 *
 * Go:
 *   gossiper := cluster.GetGossiper()
 *   gossiper.SetState("leader", leaderPid)
 *
 *
 * 6. CLUSTER ARCHITECTURE
 * -----------------------
 *
 *           ┌──────────────────────────────────────────────┐
 *           │              Cluster (Gossip)                │
 *           │                                              │
 *   ┌───────┴───────┐  ┌───────┴───────┐  ┌───────┴───────┐
 *   │    Member 1   │  │    Member 2   │  │    Member 3   │
 *   │  localhost:1  │  │  localhost:2  │  │  localhost:3  │
 *   │  ┌─────────┐  │  │  ┌─────────┐  │  │  ┌─────────┐  │
 *   │  │Grain A  │◄─┼──┼─►│Grain B  │◄─┼──┼─►│Grain C  │  │
 *   │  └─────────┘  │  │  └─────────┘  │  │  └─────────┘  │
 *   └───────────────┘  └───────────────┘  └───────────────┘
 *
 *
 * 7. CLUSTER PROVIDERS
 * --------------------
 *
 * For production, use a cluster provider for member discovery:
 * - Consul
 * - Kubernetes
 * - Custom (implement ClusterProvider interface)
 *
 */
