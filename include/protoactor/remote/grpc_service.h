#ifndef PROTOACTOR_REMOTE_GRPC_SERVICE_H
#define PROTOACTOR_REMOTE_GRPC_SERVICE_H

#include "remote.h"
#ifdef ENABLE_GRPC
#include "remote.grpc.pb.h" // Generated gRPC header
#include <grpcpp/grpcpp.h>
#endif
#include <memory>

namespace protoactor {
namespace remote {

/**
 * @brief gRPC service implementation for remote communication.
 */
#ifdef ENABLE_GRPC
class GrpcService final : public Remoting::Service {
#else
class GrpcService {
#endif
public:
    explicit GrpcService(std::shared_ptr<Remote> remote);
    
#ifdef ENABLE_GRPC
    /**
     * @brief Receive bidirectional stream for remote messages.
     */
    grpc::Status Receive(
        grpc::ServerContext* context,
        grpc::ServerReaderWriter<RemoteMessage, RemoteMessage>* stream) override;
    
    /**
     * @brief List processes.
     */
    grpc::Status ListProcesses(
        grpc::ServerContext* context,
        const ListProcessesRequest* request,
        ListProcessesResponse* response) override;
    
    /**
     * @brief Get process diagnostics.
     */
    grpc::Status GetProcessDiagnostics(
        grpc::ServerContext* context,
        const GetProcessDiagnosticsRequest* request,
        GetProcessDiagnosticsResponse* response) override;
#else
    // Stub implementations when gRPC is not enabled
    int Receive(void* context, void* stream) { return 0; }
    int ListProcesses(void* context, void* request, void* response) { return 0; }
    int GetProcessDiagnostics(void* context, void* request, void* response) { return 0; }
#endif

private:
    std::shared_ptr<Remote> remote_;
#ifdef ENABLE_GRPC
    void HandleMessageBatch(const MessageBatch& batch);
    void HandleConnectRequest(const ConnectRequest& request, ConnectResponse* response);
#endif
};

} // namespace remote
} // namespace protoactor

#endif // PROTOACTOR_REMOTE_GRPC_SERVICE_H
