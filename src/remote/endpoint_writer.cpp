#include "internal/remote/endpoint_writer.h"
#include "external/remote/remote.h"
#include "internal/remote/serializer.h"
#include "internal/remote/messages.h"
#include "external/messages.h"
#include <thread>
#include <chrono>
#include <algorithm>
#include <stdexcept>
#include <unordered_map>

#ifdef ENABLE_GRPC
#include <grpcpp/grpcpp.h>
#include "remote.grpc.pb.h"
#include "remote.pb.h"
#endif

namespace protoactor {
namespace remote {

EndpointWriter::EndpointWriter(
    std::shared_ptr<Remote> remote,
    const std::string& address,
    std::shared_ptr<Config> config)
    : remote_(std::move(remote)),
      address_(address),
      config_(std::move(config)),
      connected_(false)
#ifdef ENABLE_GRPC
      , receive_thread_running_(false)
#endif
{
}

void EndpointWriter::Receive(std::shared_ptr<Context> context) {
    auto msg = context->Message();
    
    // Try Started
    auto started = std::dynamic_pointer_cast<protoactor::Started>(
        std::static_pointer_cast<protoactor::SystemMessage>(msg));
    if (started) {
        Initialize(context);
        return;
    }
    
    // Try Stopping
    auto stopping = std::dynamic_pointer_cast<protoactor::Stopping>(
        std::static_pointer_cast<protoactor::SystemMessage>(msg));
    if (stopping) {
        HandleDisconnect();
        return;
    }
    
    // Try RemoteDeliver
    auto deliver = std::dynamic_pointer_cast<RemoteDeliver>(
        std::static_pointer_cast<SystemMessage>(msg));
    if (deliver) {
        // Queue RemoteDeliver message for batch sending
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            message_queue_.push(deliver);
        }
        
        // If queue reaches batch size, send batch
        if (message_queue_.size() >= static_cast<size_t>(config_->endpoint_writer_batch_size)) {
            SendBatch(context);
        }
        return;
    }
    
    // Try EndpointTerminatedEvent
    auto terminated = std::dynamic_pointer_cast<EndpointTerminatedEvent>(
        std::static_pointer_cast<protoactor::SystemMessage>(msg));
    if (terminated) {
        HandleDisconnect();
        return;
    }
}

void EndpointWriter::Initialize(std::shared_ptr<Context> context) {
    // Try to connect with retries
    int retry_count = 0;
    bool connected = false;
    
    while (retry_count < config_->max_retry_count && !connected) {
        connected = InitializeInternal();
        if (!connected) {
            retry_count++;
            // Wait before retry (exponential backoff would be better)
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
    
    if (!connected) {
        // Publish EndpointTerminatedEvent
        // TODO: Publish event to event stream
        return;
    }
    
    connected_.store(true, std::memory_order_release);
    
    // Start receiving thread (when gRPC is integrated)
    // receive_thread_ = std::thread([this]() { ReceiveLoop(); });
}

bool EndpointWriter::InitializeInternal() {
#ifdef ENABLE_GRPC
    try {
        // 1. Create gRPC channel
        channel_ = grpc::CreateChannel(address_, grpc::InsecureChannelCredentials());
        if (!channel_) {
            return false;
        }
        
        // 2. Create stub
        stub_ = Remoting::NewStub(channel_);
        if (!stub_) {
            return false;
        }
        
        // 3. Create bidirectional stream
        client_context_ = std::make_shared<grpc::ClientContext>();
        stream_ = stub_->Receive(client_context_.get());
        if (!stream_) {
            return false;
        }
        
        // 4. Send ConnectRequest
        RemoteMessage connect_msg;
        ConnectRequest* connect_req = connect_msg.mutable_connect_request();
        ServerConnection* server_conn = connect_req->mutable_server_connection();
        server_conn->set_member_id(remote_->GetActorSystem()->Id);
        server_conn->set_address(remote_->GetActorSystem()->Address());
        
        // Add blocklist
        if (remote_->GetBlockList()) {
            auto blocked = remote_->GetBlockList()->BlockedMembers();
            for (const auto& member_id : blocked) {
                server_conn->add_block_list(member_id);
            }
        }
        
        if (!stream_->Write(connect_msg)) {
            return false;
        }
        
        // 5. Receive ConnectResponse
        RemoteMessage response;
        if (!stream_->Read(&response)) {
            return false;
        }
        
        if (!response.has_connect_response()) {
            return false;
        }
        
        const ConnectResponse& conn_resp = response.connect_response();
        if (conn_resp.blocked()) {
            // Remote endpoint blocked us
            return false;
        }
        
        // 6. Start receive loop in background thread
        receive_thread_running_.store(true, std::memory_order_release);
        receive_thread_ = std::thread([this]() { ReceiveLoop(); });
        
        return true;
    } catch (const std::exception& e) {
        // Log error
        return false;
    }
#else
    // gRPC not enabled
    return false;
#endif
}

void EndpointWriter::SendMessageBatch(const std::vector<std::shared_ptr<RemoteDeliver>>& batch) {
    if (!connected_.load(std::memory_order_acquire) || batch.empty()) {
        return;
    }
    
#ifdef ENABLE_GRPC
    if (!stream_) {
        return;
    }
    
    // Create MessageBatch protobuf
    RemoteMessage remote_msg;
    MessageBatch* msg_batch = remote_msg.mutable_message_batch();
    
    // Collect unique type names, targets, and senders
    std::unordered_map<std::string, int32_t> type_name_map;
    std::unordered_map<std::string, int32_t> target_map;
    std::unordered_map<std::shared_ptr<protoactor::PID>, int32_t> sender_map;
    
    for (const auto& deliver : batch) {
        if (!deliver || !deliver->target) {
            continue;
        }
        
        try {
            // Serialize message
            auto [bytes, type_name] = SerializerRegistry::Serialize(
                deliver->message,
                deliver->serializer_id >= 0 ? deliver->serializer_id : -1);
            
            // Add type name if not already present
            int32_t type_id;
            auto type_it = type_name_map.find(type_name);
            if (type_it == type_name_map.end()) {
                type_id = msg_batch->type_names_size();
                msg_batch->add_type_names(type_name);
                type_name_map[type_name] = type_id;
            } else {
                type_id = type_it->second;
            }
            
            // Add target if not already present
            int32_t target_idx;
            std::string target_id = deliver->target->id;
            auto target_it = target_map.find(target_id);
            if (target_it == target_map.end()) {
                target_idx = msg_batch->targets_size();
                msg_batch->add_targets(target_id);
                target_map[target_id] = target_idx;
            } else {
                target_idx = target_it->second;
            }
            
            // Add sender if not already present
            int32_t sender_idx = 0; // 0 means no sender
            if (deliver->sender) {
                auto sender_it = sender_map.find(deliver->sender);
                if (sender_it == sender_map.end()) {
                    sender_idx = msg_batch->senders_size() + 1; // 1-based index
                    auto* sender_pid = msg_batch->add_senders();
                    sender_pid->set_address(deliver->sender->address);
                    sender_pid->set_id(deliver->sender->id);
                    sender_map[deliver->sender] = sender_idx;
                } else {
                    sender_idx = sender_it->second;
                }
            }
            
            // Create envelope
            MessageEnvelope* pb_envelope = msg_batch->add_envelopes();
            pb_envelope->set_type_id(type_id);
            pb_envelope->set_message_data(bytes.data(), bytes.size());
            pb_envelope->set_target(target_idx);
            pb_envelope->set_sender(sender_idx);
            pb_envelope->set_serializer_id(deliver->serializer_id >= 0 ? deliver->serializer_id : -1);
            
            if (deliver->target->request_id > 0) {
                pb_envelope->set_target_request_id(deliver->target->request_id);
            }
            if (deliver->sender && deliver->sender->request_id > 0) {
                pb_envelope->set_sender_request_id(deliver->sender->request_id);
            }
            
            // Add message header if present
            if (deliver->header) {
                // TODO: Serialize header
            }
        } catch (const std::exception& e) {
            // Log error and skip message
            continue;
        }
    }
    
    // Send via gRPC stream
    if (msg_batch->envelopes_size() > 0) {
        if (!stream_->Write(remote_msg)) {
            // Failed to send, disconnect
            HandleDisconnect();
        }
    }
#else
    // gRPC not enabled
#endif
}

void EndpointWriter::HandleDisconnect() {
    connected_.store(false, std::memory_order_release);
    
    CloseClientConn();
    
    // Clear message queue
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        while (!message_queue_.empty()) {
            message_queue_.pop();
        }
    }
}

void EndpointWriter::CloseClientConn() {
#ifdef ENABLE_GRPC
    receive_thread_running_.store(false, std::memory_order_release);
    
    if (stream_) {
        stream_->WritesDone();
        stream_.reset();
    }
    
    if (client_context_) {
        client_context_.reset();
    }
    
    if (stub_) {
        stub_.reset();
    }
    
    if (channel_) {
        channel_.reset();
    }
    
    if (receive_thread_.joinable()) {
        receive_thread_.join();
    }
#endif
}

void EndpointWriter::ReceiveLoop() {
#ifdef ENABLE_GRPC
    RemoteMessage msg;
    while (receive_thread_running_.load(std::memory_order_acquire) && stream_) {
        if (!stream_->Read(&msg)) {
            // Stream closed or error
            break;
        }
        
        // Handle DisconnectRequest
        if (msg.has_disconnect_request()) {
            // Remote endpoint is disconnecting
            // Publish EndpointTerminatedEvent
            auto terminated = std::make_shared<EndpointTerminatedEvent>();
            terminated->Address = address_;
            remote_->GetActorSystem()->EventStream->Publish(terminated);
            break;
        }
        
        // Other message types (ConnectResponse already handled in InitializeInternal)
    }
    
    // Connection lost
    connected_.store(false, std::memory_order_release);
    auto terminated = std::make_shared<EndpointTerminatedEvent>();
    terminated->Address = address_;
    remote_->GetActorSystem()->EventStream->Publish(terminated);
#endif
}

void EndpointWriter::SendBatch(std::shared_ptr<Context> context) {
    std::vector<std::shared_ptr<RemoteDeliver>> batch;
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        size_t batch_size = std::min(
            message_queue_.size(),
            static_cast<size_t>(config_->endpoint_writer_batch_size));
        
        for (size_t i = 0; i < batch_size; ++i) {
            if (!message_queue_.empty()) {
                batch.push_back(message_queue_.front());
                message_queue_.pop();
            }
        }
    }
    
    if (!batch.empty()) {
        SendMessageBatch(batch);
    }
}

} // namespace remote
} // namespace protoactor
