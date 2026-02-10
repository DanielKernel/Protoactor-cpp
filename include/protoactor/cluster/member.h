#ifndef PROTOACTOR_CLUSTER_MEMBER_H
#define PROTOACTOR_CLUSTER_MEMBER_H

#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace protoactor {
namespace cluster {

/**
 * @brief Member represents a cluster member.
 */
class Member {
public:
    std::string id;
    std::string host;
    int port;
    std::vector<std::string> kinds;
    std::chrono::system_clock::time_point last_heartbeat;
    bool alive;
    
    Member();
    Member(const std::string& id, const std::string& host, int port);
    
    /**
     * @brief Get the address string.
     * @return Address string
     */
    std::string Address() const;
    
    /**
     * @brief Check if member is alive.
     * @return True if alive
     */
    bool IsAlive() const;
    
    /**
     * @brief Update heartbeat.
     */
    void UpdateHeartbeat();
};

} // namespace cluster
} // namespace protoactor

#endif // PROTOACTOR_CLUSTER_MEMBER_H
