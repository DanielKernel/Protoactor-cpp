#ifndef PROTOACTOR_REMOTE_MESSAGES_H
#define PROTOACTOR_REMOTE_MESSAGES_H

#include "external/pid.h"
#include "external/messages.h"
#include <string>
#include <memory>

namespace protoactor {
namespace remote {

/**
 * @brief Event published when a remote endpoint terminates.
 */
struct EndpointTerminatedEvent : public SystemMessage {
    std::string Address;
};

/**
 * @brief Event published when a remote endpoint establishes a connection.
 */
struct EndpointConnectedEvent : public SystemMessage {
    std::string Address;
};

/**
 * @brief Internal message for remote watch.
 */
struct RemoteWatch : public SystemMessage {
    std::shared_ptr<PID> Watcher;
    std::shared_ptr<PID> Watchee;
};

/**
 * @brief Internal message for remote unwatch.
 */
struct RemoteUnwatch : public SystemMessage {
    std::shared_ptr<PID> Watcher;
    std::shared_ptr<PID> Watchee;
};

/**
 * @brief Internal message for remote message delivery.
 */
struct RemoteDeliver : public SystemMessage {
    std::shared_ptr<ReadonlyMessageHeader> header;
    std::shared_ptr<void> message;
    std::shared_ptr<PID> target;
    std::shared_ptr<PID> sender;
    int32_t serializer_id;
};

/**
 * @brief Internal message for remote terminate.
 */
struct RemoteTerminate : public SystemMessage {
    std::shared_ptr<PID> Watcher;
    std::shared_ptr<PID> Watchee;
};

/**
 * @brief Ping message for health check.
 */
struct Ping {
};

/**
 * @brief Pong response for ping.
 */
struct Pong {
};

/**
 * @brief ActorPidRequest for remote activation.
 */
struct ActorPidRequest {
    std::string name;
    std::string kind;
};

/**
 * @brief ActorPidResponse for remote activation response.
 */
struct ActorPidResponse {
    std::shared_ptr<PID> pid;
    int32_t status_code;
    
    ActorPidResponse() : status_code(0) {}
    ActorPidResponse(std::shared_ptr<PID> p, int32_t code = 0)
        : pid(p), status_code(code) {}
};

} // namespace remote
} // namespace protoactor

#endif // PROTOACTOR_REMOTE_MESSAGES_H
