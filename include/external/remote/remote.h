#ifndef PROTOACTOR_REMOTE_REMOTE_H
#define PROTOACTOR_REMOTE_REMOTE_H

#include "../actor_system.h"
#include "../props.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include <chrono>

namespace protoactor {
namespace remote {

// Forward declarations
class EndpointManager;
class Serializer;
class GrpcService;
class BlockList;

#ifdef ENABLE_GRPC
#include <grpcpp/grpcpp.h>
#endif

/**
 * @brief Remote configuration.
 */
class Config {
public:
    std::string host;
    int port;
    std::string advertised_host;
    int endpoint_writer_batch_size;
    int endpoint_writer_queue_size;
    int endpoint_manager_batch_size;
    int endpoint_manager_queue_size;
    std::unordered_map<std::string, std::shared_ptr<protoactor::Props>> kinds;
    int max_retry_count;
    
    Config();
    
    /**
     * @brief Get the address string.
     * @return Address string
     */
    std::string Address() const;
};

/**
 * @brief Remote enables communication between actor systems across network.
 */
class Remote : public std::enable_shared_from_this<Remote> {
public:
    /**
     * @brief Start the remote subsystem.
     * @param actor_system The actor system
     * @param host Host address
     * @param port Port number
     * @param config_options Configuration options
     * @return Remote instance
     */
    static std::shared_ptr<Remote> Start(
        std::shared_ptr<protoactor::ActorSystem> actor_system,
        const std::string& host,
        int port,
        std::vector<std::function<void(std::shared_ptr<Config>)>> config_options = {});
    
    /**
     * @brief Register a kind (actor type) for remote spawning.
     * @param kind Kind name
     * @param props Actor properties
     */
    void Register(const std::string& kind, std::shared_ptr<protoactor::Props> props);
    
    /**
     * @brief Spawn a remote actor.
     * @param address Remote address
     * @param kind Actor kind
     * @param name Actor name
     * @param timeout Timeout duration
     * @return PID and error code
     */
    std::pair<std::shared_ptr<protoactor::PID>, std::error_code> SpawnNamed(
        const std::string& address,
        const std::string& kind,
        const std::string& name,
        std::chrono::milliseconds timeout);
    
    /**
     * @brief Get the endpoint manager.
     * @return Endpoint manager
     */
    std::shared_ptr<EndpointManager> GetEndpointManager() const;
    
    /**
     * @brief Get the serializer.
     * @return Serializer
     */
    std::shared_ptr<Serializer> GetSerializer() const;
    
    /**
     * @brief Send a message to a remote PID.
     * @param pid Target PID
     * @param header Message header
     * @param message Message
     * @param sender Sender PID
     * @param serializer_id Serializer ID
     */
    void SendMessage(
        std::shared_ptr<protoactor::PID> pid,
        std::shared_ptr<ReadonlyMessageHeader> header,
        std::shared_ptr<void> message,
        std::shared_ptr<protoactor::PID> sender,
        int32_t serializer_id);
    
    /**
     * @brief Get the actor system.
     * @return Actor system
     */
    std::shared_ptr<protoactor::ActorSystem> GetActorSystem() const;
    
    /**
     * @brief Get the configuration.
     * @return Configuration
     */
    std::shared_ptr<Config> GetConfig() const;
    
    /**
     * @brief Get the block list.
     * @return Block list
     */
    std::shared_ptr<BlockList> GetBlockList() const;
    
    /**
     * @brief Shutdown the remote subsystem.
     * @param graceful If true, wait for running requests to finish
     */
    void Shutdown(bool graceful = true);

private:
    std::shared_ptr<protoactor::ActorSystem> actor_system_;
    std::shared_ptr<Config> config_;
    std::shared_ptr<EndpointManager> endpoint_manager_;
    std::shared_ptr<Serializer> serializer_;
    std::shared_ptr<BlockList> blocklist_;
    
#ifdef ENABLE_GRPC
    std::shared_ptr<grpc::Server> grpc_server_;
    std::shared_ptr<GrpcService> grpc_service_;
    std::thread server_thread_;
#endif
    
    Remote(std::shared_ptr<protoactor::ActorSystem> actor_system, std::shared_ptr<Config> config);
    void Initialize();
    void StartGrpcServer();
    void StopGrpcServer(bool graceful);
};

} // namespace remote
} // namespace protoactor

#endif // PROTOACTOR_REMOTE_REMOTE_H
