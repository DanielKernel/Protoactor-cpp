#ifndef PROTOACTOR_CLUSTER_CLUSTER_H
#define PROTOACTOR_CLUSTER_CLUSTER_H

#include "../actor_system.h"
#include "../remote/remote.h"
#include "../eventstream.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>

namespace protoactor {
namespace cluster {

// Forward declarations
class Cluster;
class MemberList;
class Gossiper;
class PubSub;
class PidCache;
class IdentityLookup;
class ActivatedKind;
class ClusterProvider;

/**
 * @brief Cluster configuration.
 */
class Config {
public:
    std::string cluster_name;
    std::string host;
    int port;
    std::string advertised_host;
    std::shared_ptr<remote::Config> remote_config;
    std::function<std::shared_ptr<IdentityLookup>(std::shared_ptr<Cluster>)> identity_lookup_producer;
    std::function<std::shared_ptr<void>(std::shared_ptr<Cluster>)> cluster_context_producer;
    std::shared_ptr<ClusterProvider> cluster_provider;
    
    Config();
    
    /**
     * @brief Create a new cluster configuration.
     * @param cluster_name Cluster name
     * @param host Host address
     * @param port Port number
     * @param options Configuration options
     * @return Config instance
     */
    static std::shared_ptr<Config> New(
        const std::string& cluster_name,
        const std::string& host,
        int port,
        std::vector<std::function<void(std::shared_ptr<Config>)>> options = {});
};

/**
 * @brief Cluster enables distributed actors and grain management.
 */
class Cluster {
public:
    /**
     * @brief Create a new cluster.
     * @param actor_system The actor system
     * @param config Cluster configuration
     * @return Cluster instance
     */
    static std::shared_ptr<Cluster> New(
        std::shared_ptr<protoactor::ActorSystem> actor_system,
        std::shared_ptr<Config> config);
    
    /**
     * @brief Get the actor system.
     * @return Actor system
     */
    std::shared_ptr<protoactor::ActorSystem> GetActorSystem() const;
    
    /**
     * @brief Get the member list.
     * @return Member list
     */
    std::shared_ptr<MemberList> GetMemberList() const;
    
    /**
     * @brief Get the PID cache.
     * @return PID cache
     */
    std::shared_ptr<PidCache> GetPidCache() const;
    
    /**
     * @brief Get the remote subsystem.
     * @return Remote instance
     */
    std::shared_ptr<remote::Remote> GetRemote() const;
    
    /**
     * @brief Get virtual actor count.
     * @return Total count
     */
    int64_t VirtualActorCount() const;
    
    /**
     * @brief Start the cluster.
     */
    void Start();
    
    /**
     * @brief Shutdown the cluster.
     */
    void Shutdown();

private:
    std::shared_ptr<protoactor::ActorSystem> actor_system_;
    std::shared_ptr<Config> config_;
    std::shared_ptr<Gossiper> gossip_;
    std::shared_ptr<PubSub> pubsub_;
    std::shared_ptr<remote::Remote> remote_;
    std::shared_ptr<PidCache> pid_cache_;
    std::shared_ptr<MemberList> member_list_;
    std::shared_ptr<IdentityLookup> identity_lookup_;
    std::unordered_map<std::string, std::shared_ptr<ActivatedKind>> kinds_;
    std::shared_ptr<void> context_;
    std::shared_ptr<ClusterProvider> cluster_provider_;
    std::shared_ptr<protoactor::eventstream::Subscription> topology_sub_;
    
public:
    Cluster(std::shared_ptr<protoactor::ActorSystem> actor_system, std::shared_ptr<Config> config);
private:
    void Initialize();
    void SubscribeToTopologyEvents();
};

/**
 * @brief Get the cluster from an actor system.
 * @param actor_system The actor system
 * @return Cluster instance or nullptr
 */
std::shared_ptr<Cluster> GetCluster(std::shared_ptr<protoactor::ActorSystem> actor_system);

} // namespace cluster
} // namespace protoactor

#endif // PROTOACTOR_CLUSTER_CLUSTER_H
