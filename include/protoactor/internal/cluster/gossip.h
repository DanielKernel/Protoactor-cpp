#ifndef PROTOACTOR_CLUSTER_GOSSIP_H
#define PROTOACTOR_CLUSTER_GOSSIP_H

#include "protoactor/pid.h"
#include "protoactor/messages.h"
#include "member.h"
#include "cluster_topology.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <functional>

namespace protoactor {
namespace cluster {

// Forward declarations
class Cluster;
struct ClusterTopology;

/**
 * @brief Gossip state for a member.
 */
struct GossipMemberState {
    int64_t sequence_number;
    std::chrono::system_clock::time_point heartbeat;
    std::vector<std::string> blocked;
    std::shared_ptr<void> topology; // ClusterTopology
};

/**
 * @brief Gossip state containing all members.
 */
struct GossipState {
    std::unordered_map<std::string, std::shared_ptr<GossipMemberState>> members;
};

/**
 * @brief Gossip update message.
 */
struct GossipUpdate : public SystemMessage {
    std::string key;
    std::shared_ptr<void> value;
    int64_t sequence_number;
    std::string member_id;
};

/**
 * @brief Gossip interface for state synchronization.
 */
class Gossip {
public:
    virtual ~Gossip() = default;
    
    /**
     * @brief Update cluster topology.
     * @param topology Cluster topology
     */
    virtual void UpdateClusterTopology(std::shared_ptr<ClusterTopology> topology) = 0;
    
    /**
     * @brief Get gossip state.
     * @return Gossip state
     */
    virtual std::shared_ptr<GossipState> GetState() = 0;
    
    /**
     * @brief Set state for a key.
     * @param key State key
     * @param value State value
     */
    virtual void SetState(const std::string& key, std::shared_ptr<void> value) = 0;
    
    /**
     * @brief Get state for a key.
     * @param key State key
     * @return State value
     */
    virtual std::shared_ptr<void> GetState(const std::string& key) = 0;
    
    /**
     * @brief Add consensus check.
     * @param key State key
     * @param check Consensus check function
     */
    virtual void AddConsensusCheck(const std::string& key, std::function<bool(std::shared_ptr<void>)> check) = 0;
};

/**
 * @brief Informer implements the Gossip interface.
 */
class Informer : public Gossip {
public:
    Informer(
        const std::string& my_id,
        std::function<std::vector<std::string>()> getBlockedMembers,
        int fan_out,
        int max_send,
        std::shared_ptr<Cluster> cluster);
    
    void UpdateClusterTopology(std::shared_ptr<ClusterTopology> topology) override;
    std::shared_ptr<GossipState> GetState() override;
    void SetState(const std::string& key, std::shared_ptr<void> value) override;
    std::shared_ptr<void> GetState(const std::string& key) override;
    void AddConsensusCheck(const std::string& key, std::function<bool(std::shared_ptr<void>)> check) override;

private:
    std::string my_id_;
    int64_t local_seq_number_;
    std::shared_ptr<GossipState> state_;
    std::unordered_map<std::string, int64_t> committed_offsets_;
    std::unordered_map<std::string, bool> active_member_ids_;
    std::vector<std::shared_ptr<Member>> other_members_;
    std::unordered_map<std::string, std::function<bool(std::shared_ptr<void>)>> consensus_checks_;
    std::function<std::vector<std::string>()> get_blocked_members_;
    int gossip_fan_out_;
    int gossip_max_send_;
    std::shared_ptr<Cluster> cluster_;
    
    void SendGossip();
    void ReceiveGossip(std::shared_ptr<void> update);
    void MergeState(std::shared_ptr<void> update);
    std::vector<std::shared_ptr<Member>> SelectRandomMembers(int count);
};

} // namespace cluster
} // namespace protoactor

#endif // PROTOACTOR_CLUSTER_GOSSIP_H
