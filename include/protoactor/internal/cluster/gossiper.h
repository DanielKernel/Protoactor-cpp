#ifndef PROTOACTOR_CLUSTER_GOSSIPER_H
#define PROTOACTOR_CLUSTER_GOSSIPER_H

#include "gossip.h"
#include <memory>
#include <string>
#include <vector>
#include <chrono>

namespace protoactor {
namespace cluster {

// Forward declaration
class Cluster;

/**
 * @brief Gossiper manages gossip protocol for cluster state synchronization.
 */
class Gossiper {
public:
    explicit Gossiper(std::shared_ptr<Cluster> cluster);
    ~Gossiper();
    
    /**
     * @brief Start gossiping.
     */
    void StartGossiping();
    
    /**
     * @brief Stop gossiping.
     */
    void StopGossiping();
    
    /**
     * @brief Send gossip state to other members.
     */
    void SendState();
    
    /**
     * @brief Receive gossip state from another member.
     * @param member_id Member ID
     * @param state Gossip state
     */
    void ReceiveState(const std::string& member_id, std::shared_ptr<void> state);

private:
    std::shared_ptr<Cluster> cluster_;
    std::shared_ptr<Gossip> gossip_;
    std::atomic<bool> running_;
    std::chrono::milliseconds gossip_interval_;
    
    void GossipLoop();
};

} // namespace cluster
} // namespace protoactor

#endif // PROTOACTOR_CLUSTER_GOSSIPER_H
