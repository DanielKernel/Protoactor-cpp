#ifndef PROTOACTOR_CLUSTER_MEMBER_LIST_H
#define PROTOACTOR_CLUSTER_MEMBER_LIST_H

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>
#include "protoactor/eventstream.h"

namespace protoactor {
namespace cluster {

// Forward declarations
class Cluster;
class Member;
class MemberStrategy;
struct ClusterTopology;
struct GossipUpdate;

/**
 * @brief MemberList keeps track of the current cluster topology.
 */
class MemberList {
public:
    explicit MemberList(std::shared_ptr<Cluster> cluster);
    ~MemberList();
    
    /**
     * @brief Get all members.
     * @return Vector of members
     */
    std::vector<std::shared_ptr<Member>> GetMembers() const;
    
    /**
     * @brief Get a member by ID.
     * @param member_id Member ID
     * @return Member or nullptr
     */
    std::shared_ptr<Member> GetMember(const std::string& member_id) const;
    
    /**
     * @brief Update the member list.
     * @param topology Cluster topology
     */
    void UpdateTopology(std::shared_ptr<ClusterTopology> topology);
    
    /**
     * @brief Initialize topology consensus.
     */
    void InitializeTopologyConsensus();

private:
    std::shared_ptr<Cluster> cluster_;
    mutable std::mutex mutex_;
    std::vector<std::shared_ptr<Member>> members_;
    std::unordered_map<std::string, std::shared_ptr<MemberStrategy>> member_strategy_by_kind_;
    std::shared_ptr<protoactor::eventstream::Subscription> gossip_sub_; // Subscription to GossipUpdate events
    
    void HandleGossipUpdate(std::shared_ptr<void> update);
};

} // namespace cluster
} // namespace protoactor

#endif // PROTOACTOR_CLUSTER_MEMBER_LIST_H
