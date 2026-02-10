#include "protoactor/remote/grpc_service.h"
#include "protoactor/remote/remote.h"
#include "protoactor/remote/endpoint_reader.h"
#include "protoactor/remote/serializer.h"
#include "protoactor/actor_system.h"
#include "protoactor/process_registry.h"
#include <stdexcept>

namespace protoactor {
namespace remote {

GrpcService::GrpcService(std::shared_ptr<Remote> remote)
    : remote_(remote) {
}

grpc::Status GrpcService::Receive(
    grpc::ServerContext* context,
    grpc::ServerReaderWriter<RemoteMessage, RemoteMessage>* stream) {
    
    if (!remote_) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Remote not initialized");
    }
    
    // Store connection for disconnect handling
    // TODO: Store in EndpointManager for disconnect management
    
    // Receive loop
    RemoteMessage msg;
    while (stream->Read(&msg)) {
        if (msg.has_message_batch()) {
            HandleMessageBatch(msg.message_batch());
        } else if (msg.has_connect_request()) {
            ConnectResponse response;
            HandleConnectRequest(msg.connect_request(), &response);
            
            RemoteMessage response_msg;
            *response_msg.mutable_connect_response() = response;
            if (!stream->Write(response_msg)) {
                // Failed to write response
                break;
            }
        } else if (msg.has_disconnect_request()) {
            // Client is disconnecting
            // TODO: Notify EndpointManager
            break;
        }
    }
    
    return grpc::Status::OK;
}

grpc::Status GrpcService::ListProcesses(
    grpc::ServerContext* context,
    const ListProcessesRequest* request,
    ListProcessesResponse* response) {
    
    // TODO: Implement process listing
    // Would query ProcessRegistry for matching PIDs
    return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Not implemented");
}

grpc::Status GrpcService::GetProcessDiagnostics(
    grpc::ServerContext* context,
    const GetProcessDiagnosticsRequest* request,
    GetProcessDiagnosticsResponse* response) {
    
    // TODO: Implement diagnostics
    return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Not implemented");
}

void GrpcService::HandleMessageBatch(const MessageBatch& batch) {
    if (!remote_) {
        return;
    }
    
    // Create endpoint reader to process the batch
    auto endpoint_reader = std::make_shared<EndpointReader>(remote_);
    
    // Process each envelope in the batch
    for (int i = 0; i < batch.envelopes_size(); ++i) {
        const auto& envelope = batch.envelopes(i);
        
        // Deserialize target PID (targets is a string array of IDs)
        std::shared_ptr<protoactor::PID> target;
        if (envelope.target() >= 0 && envelope.target() < batch.targets_size()) {
            const std::string& target_id = batch.targets(envelope.target());
            std::string address = remote_->GetActorSystem()->Address();
            target = std::make_shared<protoactor::PID>(address, target_id);
            if (envelope.target_request_id() > 0) {
                target->RequestId = envelope.target_request_id();
            }
        }
        
        // Deserialize sender PID (senders is a PID array)
        std::shared_ptr<protoactor::PID> sender;
        if (envelope.sender() > 0 && envelope.sender() <= batch.senders_size()) {
            // sender index is 1-based (0 means no sender)
            const auto& sender_pid = batch.senders(envelope.sender() - 1);
            sender = std::make_shared<protoactor::PID>(
                sender_pid.address(), sender_pid.id());
            if (envelope.sender_request_id() > 0) {
                sender->RequestId = envelope.sender_request_id();
            }
        }
        
        // Get type name
        std::string type_name;
        if (envelope.type_id() >= 0 && envelope.type_id() < batch.type_names_size()) {
            type_name = batch.type_names(envelope.type_id());
        }
        
        // Deserialize and deliver message
        if (target && !type_name.empty()) {
            std::vector<uint8_t> data(envelope.message_data().begin(), 
                                      envelope.message_data().end());
            endpoint_reader->DeserializeAndDeliver(
                data, type_name, envelope.serializer_id(), target, sender);
        }
    }
}

void GrpcService::HandleConnectRequest(const ConnectRequest& request, ConnectResponse* response) {
    if (!remote_) {
        return;
    }
    
    // Handle server connection (incoming connection from remote endpoint)
    if (request.has_server_connection()) {
        const auto& server_conn = request.server_connection();
        
        // Check blocklist
        bool blocked = false;
        if (remote_->GetBlockList()) {
            blocked = remote_->GetBlockList()->IsBlocked(server_conn.member_id());
        }
        
        // TODO: Store connection info in EndpointManager
        
        // Set response
        response->set_member_id(remote_->GetActorSystem()->Id);
        response->set_blocked(blocked);
    } else if (request.has_client_connection()) {
        // Client connection (outgoing connection)
        // TODO: Handle client connection
        response->set_member_id(remote_->GetActorSystem()->Id);
        response->set_blocked(false);
    }
}

} // namespace remote
} // namespace protoactor
