#include "external/cluster/cluster.h"
#include "internal/cluster/member_list.h"
#include "internal/cluster/cluster_topology.h"
#include "internal/cluster/pid_cache.h"
#include "internal/cluster/gossiper.h"
#include "internal/cluster/pubsub.h"
#include "external/remote/remote.h"
#include "external/messages.h"
#include "external/extensions.h"
#include <stdexcept>

namespace protoactor {
namespace cluster {


Config::Config()
    : cluster_name(""),
      host("localhost"),
      port(8080),
      advertised_host("") {
}

std::shared_ptr<Config> Config::New(
    const std::string& cluster_name,
    const std::string& host,
    int port,
    std::vector<std::function<void(std::shared_ptr<Config>)>> options) {
    
    auto config = std::make_shared<Config>();
    config->cluster_name = cluster_name;
    config->host = host;
    config->port = port;
    
    for (auto& opt : options) {
        opt(config);
    }
    
    return config;
}

Cluster::Cluster(
    std::shared_ptr<protoactor::ActorSystem> actor_system,
    std::shared_ptr<Config> config)
    : actor_system_(std::move(actor_system)),
      config_(std::move(config)) {
    // Initialize will be called after object is created
}

std::shared_ptr<Cluster> Cluster::New(
    std::shared_ptr<protoactor::ActorSystem> actor_system,
    std::shared_ptr<Config> config) {
    auto cluster = std::make_shared<Cluster>(actor_system, config);
    
    // Now initialize with proper shared_ptr
    cluster->member_list_ = std::make_shared<MemberList>(cluster);
    cluster->gossip_ = std::make_shared<Gossiper>(cluster);
    cluster->pubsub_ = std::make_shared<PubSub>(cluster);
    
    return cluster;
}

void Cluster::Initialize() {
    // Initialize PID cache
    pid_cache_ = std::make_shared<PidCache>();
    
    // Member list, gossip, and pubsub are already initialized in New() method
    // after the shared_ptr is created
    
    // Subscribe to topology events
    SubscribeToTopologyEvents();
}

void Cluster::SubscribeToTopologyEvents() {
    // Subscribe to ClusterTopology events
    auto event_stream = actor_system_->GetEventStream();
    topology_sub_ = event_stream->SubscribeWithPredicate(
        [this](std::shared_ptr<void> evt) {
            if (auto topology = std::dynamic_pointer_cast<ClusterTopology>(
                    std::static_pointer_cast<SystemMessage>(evt))) {
                // Remove left members from PID cache
                auto cache = GetPidCache();
                if (cache) {
                    for (const auto& member : topology->left) {
                        // Clear cache for left members
                        // In full implementation, would remove all PIDs for that member
                    }
                }
            }
        },
        [](std::shared_ptr<void> evt) {
            return std::dynamic_pointer_cast<ClusterTopology>(
                std::static_pointer_cast<SystemMessage>(evt)) != nullptr;
        }
    );
}

std::shared_ptr<protoactor::ActorSystem> Cluster::GetActorSystem() const {
    return actor_system_;
}

std::shared_ptr<MemberList> Cluster::GetMemberList() const {
    return member_list_;
}

std::shared_ptr<PidCache> Cluster::GetPidCache() const {
    return pid_cache_;
}

std::shared_ptr<remote::Remote> Cluster::GetRemote() const {
    return remote_;
}

int64_t Cluster::VirtualActorCount() const {
    int64_t total = 0;
    // TODO: Sum up counts from all kinds
    // for (const auto& [kind, activated] : kinds_) {
    //     total += activated->Count();
    // }
    return total;
}

void Cluster::Start() {
    // Start remote subsystem
    if (!config_->remote_config) {
        config_->remote_config = std::make_shared<remote::Config>();
        config_->remote_config->host = config_->host;
        config_->remote_config->port = config_->port;
    }
    
    remote_ = remote::Remote::Start(actor_system_, config_->host, config_->port);
    
    // Initialize kinds
    // initKinds();
    
    // Start remote
    // remote_->Start();
    
    // Setup identity lookup
    // if (config_->identity_lookup_producer) {
    //     identity_lookup_ = config_->identity_lookup_producer(shared_from_this());
    //     identity_lookup_->Setup(shared_from_this(), GetClusterKinds(), false);
    // }
    
    // Start gossip
    gossip_->StartGossiping();
    
    // Start PubSub
    pubsub_->Start();
    
    // Initialize topology consensus
    // member_list_->InitializeTopologyConsensus();
    
    // Start cluster provider
    // config_->cluster_provider->StartMember(shared_from_this());
}

void Cluster::Shutdown() {
    // Stop cluster provider
    // config_->cluster_provider->Shutdown();
    
    // Stop PubSub
    pubsub_->Stop();
    
    // Stop gossip
    gossip_->StopGossiping();
    
    // Shutdown remote
    // remote_->Shutdown(true);
}

std::shared_ptr<Cluster> GetCluster(std::shared_ptr<protoactor::ActorSystem> actor_system) {
    // TODO: Get cluster from extensions
    // return actor_system->Extensions->Get<Cluster>();
    return nullptr;
}

} // namespace cluster
} // namespace protoactor
