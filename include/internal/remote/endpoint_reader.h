#ifndef PROTOACTOR_REMOTE_ENDPOINT_READER_H
#define PROTOACTOR_REMOTE_ENDPOINT_READER_H

#include "external/actor.h"
#include "external/context.h"
#include "external/pid.h"
#include <memory>
#include <string>
#include <atomic>

namespace protoactor {
namespace remote {

// Forward declarations
class Remote;

/**
 * @brief EndpointReader receives messages from remote endpoints via gRPC.
 * 
 * This class implements the gRPC Remoting service Receive method.
 * It will be fully implemented when gRPC is integrated.
 */
class EndpointReader {
public:
    explicit EndpointReader(std::shared_ptr<Remote> remote);
    
    /**
     * @brief Handle incoming message batch.
     * @param batch Message batch
     */
    void OnMessageBatch(std::shared_ptr<void> batch); // Will use protobuf MessageBatch when gRPC is integrated
    
    /**
     * @brief Handle connect request.
     * @param request Connect request
     * @return Connect response
     */
    std::shared_ptr<void> OnConnectRequest(std::shared_ptr<void> request); // Will use protobuf ConnectRequest/Response
    
    /**
     * @brief Suspend message processing.
     * @param suspend True to suspend, false to resume
     */
    void Suspend(bool suspend);

private:
    std::shared_ptr<Remote> remote_;
    std::atomic<bool> suspended_;
    
    void HandleServerConnection(std::shared_ptr<void> connection);
    void DeserializeAndDeliver(
        const std::vector<uint8_t>& data,
        const std::string& type_name,
        int32_t serializer_id,
        std::shared_ptr<PID> target,
        std::shared_ptr<PID> sender);
    
    bool IsSystemMessage(std::shared_ptr<void> message);
};

} // namespace remote
} // namespace protoactor

#endif // PROTOACTOR_REMOTE_ENDPOINT_READER_H
