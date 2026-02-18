#include "internal/cluster/gossip.h"
#include "external/cluster/cluster.h"
#include "internal/cluster/member_list.h"
#include <algorithm>
#include <random>
#include <chrono>

namespace protoactor {
namespace cluster {

Informer::Informer(
    const std::string& my_id,
    std::function<std::vector<std::string>()> getBlockedMembers,
    int fan_out,
    int max_send,
    std::shared_ptr<Cluster> cluster)
    : my_id_(my_id),
      local_seq_number_(0),
      state_(std::make_shared<GossipState>()),
      get_blocked_members_(getBlockedMembers),
      gossip_fan_out_(fan_out),
      gossip_max_send_(max_send),
      cluster_(cluster) {
}

void Informer::UpdateClusterTopology(std::shared_ptr<ClusterTopology> topology) {
    if (!topology) {
        return;
    }
    
    // Update active member IDs
    active_member_ids_.clear();
    for (const auto& member : topology->members) {
        active_member_ids_[member->id] = true;
    }
    
    // Update other members
    other_members_ = topology->members;
    
    // Remove members that left
    std::vector<std::string> to_remove;
    for (const auto& [id, _] : state_->members) {
        if (id != my_id_ && active_member_ids_.find(id) == active_member_ids_.end()) {
            to_remove.push_back(id);
        }
    }
    for (const auto& id : to_remove) {
        state_->members.erase(id);
    }
    
    // Update local state
    local_seq_number_++;
    auto my_state = std::make_shared<GossipMemberState>();
    my_state->sequence_number = local_seq_number_;
    my_state->heartbeat = std::chrono::system_clock::now();
    my_state->blocked = get_blocked_members_();
    my_state->topology = topology;
    state_->members[my_id_] = my_state;
}

std::shared_ptr<GossipState> Informer::GetState() {
    return state_;
}

void Informer::SetState(const std::string& key, std::shared_ptr<void> value) {
    local_seq_number_++;
    auto my_state = state_->members[my_id_];
    if (!my_state) {
        my_state = std::make_shared<GossipMemberState>();
        state_->members[my_id_] = my_state;
    }
    my_state->sequence_number = local_seq_number_;
    my_state->heartbeat = std::chrono::system_clock::now();
    
    // Store state by key (simplified - in full implementation would use proper storage)
}

std::shared_ptr<void> Informer::GetState(const std::string& key) {
    // Get state by key (simplified)
    return nullptr;
}

void Informer::AddConsensusCheck(const std::string& key, std::function<bool(std::shared_ptr<void>)> check) {
    consensus_checks_[key] = check;
}

void Informer::SendGossip() {
    if (other_members_.empty()) {
        return;
    }
    
    // Select random members to gossip with
    auto targets = SelectRandomMembers(gossip_fan_out_);
    
    // Create gossip update
    auto update = std::make_shared<GossipUpdate>();
    update->key = "topology";
    update->sequence_number = local_seq_number_;
    update->member_id = my_id_;
    
    // Send to selected members (simplified - would use remote messaging)
    for (const auto& member : targets) {
        // TODO: Send gossip update via remote
    }
}

void Informer::ReceiveGossip(std::shared_ptr<void> update) {
    MergeState(update);
}

void Informer::MergeState(std::shared_ptr<void> update) {
    // Merge received state with local state
    // In full implementation, this would merge sequence numbers and resolve conflicts
    if (auto gossip_update = std::dynamic_pointer_cast<GossipUpdate>(
            std::static_pointer_cast<SystemMessage>(update))) {
        auto member_id = gossip_update->member_id;
        auto seq = gossip_update->sequence_number;
        
        auto it = state_->members.find(member_id);
        if (it == state_->members.end() || it->second->sequence_number < seq) {
            // Update state
            auto member_state = std::make_shared<GossipMemberState>();
            member_state->sequence_number = seq;
            member_state->heartbeat = std::chrono::system_clock::now();
            state_->members[member_id] = member_state;
        }
    }
}

std::vector<std::shared_ptr<Member>> Informer::SelectRandomMembers(int count) {
    if (other_members_.empty()) {
        return {};
    }
    
    std::vector<std::shared_ptr<Member>> result;
    std::vector<std::shared_ptr<Member>> available = other_members_;
    
    // Random selection
    std::random_device rd;
    std::mt19937 gen(rd());
    
    int select_count = std::min(count, static_cast<int>(available.size()));
    for (int i = 0; i < select_count; ++i) {
        if (available.empty()) {
            break;
        }
        std::uniform_int_distribution<> dis(0, available.size() - 1);
        int idx = dis(gen);
        result.push_back(available[idx]);
        available.erase(available.begin() + idx);
    }
    
    return result;
}

} // namespace cluster
} // namespace protoactor
