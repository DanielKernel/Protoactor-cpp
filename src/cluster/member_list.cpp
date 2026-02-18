#include "protoactor/internal/cluster/member_list.h"
#include "protoactor/cluster/cluster.h"
#include "protoactor/internal/cluster/member.h"
#include "protoactor/internal/cluster/cluster_topology.h"
#include "protoactor/internal/cluster/gossip.h"
#include "protoactor/eventstream.h"
#include "protoactor/messages.h"
#include "protoactor/internal/log.h"
#include <algorithm>

namespace protoactor {
namespace cluster {

// ClusterTopology is now defined in cluster_topology.h

MemberList::MemberList(std::shared_ptr<Cluster> cluster)
    : cluster_(std::move(cluster)) {
    
    // Subscribe to gossip updates
    auto event_stream = cluster_->GetActorSystem()->GetEventStream();
    gossip_sub_ = event_stream->SubscribeWithPredicate(
        [this](std::shared_ptr<void> evt) {
            HandleGossipUpdate(evt);
        },
        [](std::shared_ptr<void> evt) {
            return std::dynamic_pointer_cast<GossipUpdate>(
                std::static_pointer_cast<SystemMessage>(evt)) != nullptr;
        }
    );
}

MemberList::~MemberList() {
    // Unsubscribe from gossip updates
    if (gossip_sub_) {
        cluster_->GetActorSystem()->GetEventStream()->Unsubscribe(gossip_sub_);
        gossip_sub_ = nullptr;
    }
}

std::vector<std::shared_ptr<Member>> MemberList::GetMembers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return members_;
}

std::shared_ptr<Member> MemberList::GetMember(const std::string& member_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& member : members_) {
        if (member->id == member_id) {
            return member;
        }
    }
    return nullptr;
}

void MemberList::UpdateTopology(std::shared_ptr<ClusterTopology> topology) {
    if (!topology) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Update members
    members_ = topology->members;
    
    // Update blocked members in remote
    // TODO: Implement BlockList in Remote
    // if (cluster_->GetRemote()) {
    //     cluster_->GetRemote()->BlockList()->Block(topology->blocked);
    // }
    
    // Publish topology event
    auto event_stream = cluster_->GetActorSystem()->GetEventStream();
    event_stream->Publish(std::static_pointer_cast<void>(topology));
    
    // Logger not available, skip logging for now
    // cluster_->GetActorSystem()->Logger()->Info("Updated ClusterTopology",
    //                                            slog::Int("members", topology->members.size()),
    //                                            slog::Int("left", topology->left.size()),
    //                                            slog::Int("blocked", topology->blocked.size()));
}

void MemberList::InitializeTopologyConsensus() {
    // TODO: Initialize consensus checks for topology hash
    // This ensures all members agree on the cluster topology
}

void MemberList::HandleGossipUpdate(std::shared_ptr<void> update) {
    auto gossip_update = std::dynamic_pointer_cast<GossipUpdate>(
        std::static_pointer_cast<SystemMessage>(update));
    if (!gossip_update || gossip_update->key != "topology") {
        return;
    }
    
    // Extract ClusterTopology from gossip update value
    // In full implementation, this would deserialize from protobuf Any
    // For now, if value contains ClusterTopology, use it
    // Note: We use static_pointer_cast because we know the type based on the key
    // In a full implementation, we would use type information or a type registry
    if (gossip_update->key == "topology" && gossip_update->value) {
        // Cast from void* to ClusterTopology* (unsafe but necessary for now)
        // In full implementation, we would use type information
        auto topology = std::static_pointer_cast<ClusterTopology>(
            std::static_pointer_cast<SystemMessage>(gossip_update->value));
        
        if (topology) {
            // Update blocked members in remote
            if (cluster_->GetRemote()) {
                // TODO: Implement BlockList in Remote
                // cluster_->GetRemote()->BlockList()->Block(topology->blocked);
            }
            
            // Update member list
            UpdateTopology(topology);
        }
    }
}

} // namespace cluster
} // namespace protoactor
