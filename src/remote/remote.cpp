#include "external/remote/remote.h"
#include "internal/remote/endpoint_manager.h"
#include "internal/remote/serializer.h"
#include "internal/remote/grpc_service.h"
#include "internal/remote/blocklist.h"
#include "external/actor_system.h"
#include "external/props.h"
#include "external/pid.h"
#include "internal/log.h"
#include <sstream>
#include <thread>
#include <stdexcept>

#ifdef ENABLE_GRPC
#include <grpcpp/grpcpp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

namespace protoactor {
namespace remote {

Config::Config()
    : host("localhost"),
      port(8080),
      advertised_host(""),
      endpoint_writer_batch_size(1000),
      endpoint_writer_queue_size(1000000),
      endpoint_manager_batch_size(1000),
      endpoint_manager_queue_size(1000000),
      max_retry_count(5) {
}

std::string Config::Address() const {
    std::stringstream ss;
    ss << host << ":" << port;
    return ss.str();
}

Remote::Remote(std::shared_ptr<protoactor::ActorSystem> actor_system, std::shared_ptr<Config> config)
    : actor_system_(actor_system), config_(config), blocklist_(BlockList::New()) {
}

std::shared_ptr<Remote> Remote::Start(
    std::shared_ptr<protoactor::ActorSystem> actor_system,
    const std::string& host,
    int port,
    std::vector<std::function<void(std::shared_ptr<Config>)>> config_options) {
    
    auto config = std::make_shared<Config>();
    config->host = host;
    config->port = port;
    
    for (auto& opt : config_options) {
        opt(config);
    }
    
    auto remote = std::shared_ptr<Remote>(new Remote(actor_system, config));
    remote->Initialize();
    
    // Initialize endpoint manager after remote is created
    remote->endpoint_manager_ = std::make_shared<EndpointManager>(remote);
    
    // Start gRPC server if enabled
#ifdef ENABLE_GRPC
    remote->StartGrpcServer();
#endif
    
    return remote;
}

void Remote::Initialize() {
    // Initialize remote subsystem
    // Set up address resolver for remote PIDs
    std::string address = config_->Address();
    // Address is set during ProcessRegistry creation
    // No need to set it again here
    
    // Initialize serializer (will be implemented when protobuf serializer is added)
    // serializer_ = std::make_shared<ProtoSerializer>();
}

void Remote::StartGrpcServer() {
#ifdef ENABLE_GRPC
    std::string address = config_->Address();
    
    // Create gRPC service
    grpc_service_ = std::make_shared<GrpcService>(shared_from_this());
    
    // Build server
    grpc::ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(grpc_service_.get());
    
    grpc_server_ = builder.BuildAndStart();
    if (!grpc_server_) {
        throw std::runtime_error("Failed to start gRPC server on " + address);
    }
    
    // Start server in background thread
    server_thread_ = std::thread([this]() {
        grpc_server_->Wait();
    });
    protoactor::log::GetLogger("remote")->info("gRPC server started on " + address);
#else
    // gRPC not enabled, skip server startup
#endif
}

void Remote::StopGrpcServer(bool graceful) {
#ifdef ENABLE_GRPC
    if (grpc_server_) {
        if (graceful) {
            grpc_server_->Shutdown();
        } else {
            grpc_server_->Shutdown();
        }
        
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        
        grpc_server_.reset();
        grpc_service_.reset();
    }
#endif
}

void Remote::Shutdown(bool graceful) {
    if (endpoint_manager_) {
        endpoint_manager_->Stop();
    }
    
    StopGrpcServer(graceful);
}

void Remote::Register(const std::string& kind, std::shared_ptr<protoactor::Props> props) {
    config_->kinds[kind] = props;
}

std::pair<std::shared_ptr<protoactor::PID>, std::error_code> Remote::SpawnNamed(
    const std::string& address,
    const std::string& kind,
    const std::string& name,
    std::chrono::milliseconds timeout) {
    
    // Simplified implementation - would use gRPC to spawn remote actor
    // For now, return error
    return {nullptr, std::make_error_code(std::errc::not_supported)};
}

std::shared_ptr<EndpointManager> Remote::GetEndpointManager() const {
    return endpoint_manager_;
}

std::shared_ptr<Serializer> Remote::GetSerializer() const {
    return serializer_;
}

void Remote::SendMessage(
    std::shared_ptr<protoactor::PID> pid,
    std::shared_ptr<ReadonlyMessageHeader> header,
    std::shared_ptr<void> message,
    std::shared_ptr<protoactor::PID> sender,
    int32_t serializer_id) {
    
    if (!pid) {
        return;
    }
    
    if (pid->address.empty()) {
        // Local message, send directly
        auto result = actor_system_->GetProcessRegistry()->GetLocal(pid->id);
        auto process = result.first;
        bool found = result.second;
        if (found && process) {
            if (header || sender) {
                auto envelope = std::make_shared<MessageEnvelope>(header, message, sender);
                process->SendUserMessage(pid, envelope);
            } else {
                process->SendUserMessage(pid, message);
            }
        }
        return;
    }
    
    // Remote message, use endpoint manager
    if (endpoint_manager_) {
        endpoint_manager_->RemoteDeliver(pid, message, sender);
    }
}

std::shared_ptr<protoactor::ActorSystem> Remote::GetActorSystem() const {
    return actor_system_;
}

std::shared_ptr<Config> Remote::GetConfig() const {
    return config_;
}

std::shared_ptr<BlockList> Remote::GetBlockList() const {
    return blocklist_;
}

} // namespace remote
} // namespace protoactor
