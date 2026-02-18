#ifndef PROTOACTOR_CLUSTER_CLUSTER_PROVIDER_H
#define PROTOACTOR_CLUSTER_CLUSTER_PROVIDER_H

#include "../../external/cluster/cluster.h"
#include "member.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <chrono>

namespace protoactor {
namespace cluster {

// Forward declarations
class Cluster;

/**
 * @brief ClusterProvider provides cluster membership discovery.
 */
class ClusterProvider {
public:
    virtual ~ClusterProvider() = default;
    
    /**
     * @brief Start the cluster provider as a member.
     * @param cluster Cluster instance
     * @return Error code
     */
    virtual std::error_code StartMember(std::shared_ptr<Cluster> cluster) = 0;
    
    /**
     * @brief Start the cluster provider as a client.
     * @param cluster Cluster instance
     * @return Error code
     */
    virtual std::error_code StartClient(std::shared_ptr<Cluster> cluster) = 0;
    
    /**
     * @brief Shutdown the cluster provider.
     */
    virtual void Shutdown() = 0;
    
    /**
     * @brief Register member update callback.
     * @param callback Callback function
     */
    virtual void RegisterMemberUpdateCallback(
        std::function<void(const std::vector<std::shared_ptr<Member>>&,
                          const std::vector<std::shared_ptr<Member>>&,
                          const std::vector<std::shared_ptr<Member>>&)> callback) = 0;
};

/**
 * @brief Consul cluster provider.
 */
class ConsulProvider : public ClusterProvider {
public:
    ConsulProvider(
        const std::string& consul_address = "localhost:8500",
        const std::string& datacenter = "dc1");
    
    std::error_code StartMember(std::shared_ptr<Cluster> cluster) override;
    std::error_code StartClient(std::shared_ptr<Cluster> cluster) override;
    void Shutdown() override;
    void RegisterMemberUpdateCallback(
        std::function<void(const std::vector<std::shared_ptr<Member>>&,
                          const std::vector<std::shared_ptr<Member>>&,
                          const std::vector<std::shared_ptr<Member>>&)> callback) override;

private:
    std::string consul_address_;
    std::string datacenter_;
    std::shared_ptr<Cluster> cluster_;
    std::function<void(const std::vector<std::shared_ptr<Member>>&,
                      const std::vector<std::shared_ptr<Member>>&,
                      const std::vector<std::shared_ptr<Member>>&)> member_update_callback_;
    std::atomic<bool> running_;
    
    void PollMembers();
    std::vector<std::shared_ptr<Member>> GetMembersFromConsul();
};

} // namespace cluster
} // namespace protoactor

#endif // PROTOACTOR_CLUSTER_CLUSTER_PROVIDER_H
