#ifndef PROTOACTOR_REMOTE_ENDPOINT_WRITER_H
#define PROTOACTOR_REMOTE_ENDPOINT_WRITER_H

#include "../actor.h"
#include "../context.h"
#include "../pid.h"
#include "../messages.h"
#include "messages.h" // For RemoteDeliver
#include <memory>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>

#ifdef ENABLE_GRPC
#include <grpcpp/grpcpp.h>
#include "remote.grpc.pb.h"
#endif

namespace protoactor {
namespace remote {

// Forward declarations
class Remote;
class Config;

/**
 * @brief EndpointWriter sends messages to a remote endpoint.
 */
class EndpointWriter : public Actor {
public:
    EndpointWriter(
        std::shared_ptr<Remote> remote,
        const std::string& address,
        std::shared_ptr<Config> config);
    
    void Receive(std::shared_ptr<Context> context) override;

private:
    std::shared_ptr<Remote> remote_;
    std::string address_;
    std::shared_ptr<Config> config_;
    
#ifdef ENABLE_GRPC
    std::shared_ptr<grpc::ClientContext> client_context_;
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<Remoting::Stub> stub_;
    std::unique_ptr<grpc::ClientReaderWriter<RemoteMessage, RemoteMessage>> stream_;
    std::thread receive_thread_;
    std::atomic<bool> receive_thread_running_;
#endif
    
    std::atomic<bool> connected_;
    std::queue<std::shared_ptr<RemoteDeliver>> message_queue_;
    std::mutex queue_mutex_;
    
    void Initialize(std::shared_ptr<Context> context);
    bool InitializeInternal();
    void SendMessageBatch(const std::vector<std::shared_ptr<RemoteDeliver>>& batch);
    void HandleDisconnect();
    void SendBatch(std::shared_ptr<Context> context);
    void ReceiveLoop();
    void CloseClientConn();
};

} // namespace remote
} // namespace protoactor

#endif // PROTOACTOR_REMOTE_ENDPOINT_WRITER_H
