#include "protoactor/cluster/gossiper.h"
#include "protoactor/cluster/cluster.h"
#include "protoactor/cluster/member_list.h"
#include "protoactor/cluster/gossip.h"
#include "protoactor/cluster/cluster_topology.h"
#include <thread>
#include <chrono>

namespace protoactor {
namespace cluster {

Gossiper::Gossiper(std::shared_ptr<Cluster> cluster)
    : cluster_(std::move(cluster)),
      running_(false),
      gossip_interval_(std::chrono::milliseconds(1000)) {
    // Initialize gossip implementation
    auto getBlockedMembers = [this]() -> std::vector<std::string> {
        if (cluster_) {
            auto remote = cluster_->GetRemote();
            if (remote) {
                // TODO: Get blocked members from remote
                return {};
            }
        }
        return {};
    };
    
    gossip_ = std::make_shared<Informer>(
        cluster_->GetActorSystem()->GetProcessRegistry()->Address(),
        getBlockedMembers,
        3, // fan out
        50, // max send
        cluster_);
}

Gossiper::~Gossiper() {
    StopGossiping();
}

void Gossiper::StartGossiping() {
    if (running_.load(std::memory_order_acquire)) {
        return;
    }
    
    running_.store(true, std::memory_order_release);
    
    // Start gossip loop in background thread
    std::thread([this]() {
        GossipLoop();
    }).detach();
}

void Gossiper::StopGossiping() {
    running_.store(false, std::memory_order_release);
}

void Gossiper::GossipLoop() {
    while (running_.load(std::memory_order_acquire)) {
        SendState();
        std::this_thread::sleep_for(gossip_interval_);
    }
}

void Gossiper::SendState() {
    if (gossip_) {
        auto informer = std::dynamic_pointer_cast<Informer>(gossip_);
        if (informer) {
            // Get current topology
            auto member_list = cluster_->GetMemberList();
            if (member_list) {
                auto members = member_list->GetMembers();
                // Create topology (simplified)
                auto topology = std::make_shared<ClusterTopology>();
                topology->members = members;
                informer->UpdateClusterTopology(topology);
            }
        }
    }
}

void Gossiper::ReceiveState(const std::string& member_id, std::shared_ptr<void> state) {
    // TODO: When gossip is fully implemented:
    // 1. Merge received state with local state
    // 2. Update member list if needed
    // 3. Trigger consensus checks
}

} // namespace cluster
} // namespace protoactor
