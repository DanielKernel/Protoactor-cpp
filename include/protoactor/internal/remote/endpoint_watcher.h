#ifndef PROTOACTOR_REMOTE_ENDPOINT_WATCHER_H
#define PROTOACTOR_REMOTE_ENDPOINT_WATCHER_H

#include "protoactor/actor.h"
#include "protoactor/context.h"
#include "protoactor/pid.h"
#include "protoactor/remote/remote.h"
#include "protoactor/messages.h"
#include "protoactor/internal/remote/messages.h" // For RemoteWatch, RemoteUnwatch, RemoteTerminate
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace protoactor {
namespace remote {

/**
 * @brief EndpointWatcher manages watches for a remote endpoint.
 */
class EndpointWatcher : public Actor {
public:
    EndpointWatcher(std::shared_ptr<Remote> remote, const std::string& address);
    
    void Receive(std::shared_ptr<Context> context) override;

private:
    std::shared_ptr<Remote> remote_;
    std::string address_;
    std::unordered_map<std::string, std::vector<std::shared_ptr<PID>>> watched_;
    
    void HandleRemoteWatch(std::shared_ptr<Context> context, std::shared_ptr<RemoteWatch> watch);
    void HandleRemoteUnwatch(std::shared_ptr<Context> context, std::shared_ptr<RemoteUnwatch> unwatch);
    void HandleRemoteTerminate(std::shared_ptr<Context> context, std::shared_ptr<RemoteTerminate> terminate);
    void HandleEndpointTerminated(std::shared_ptr<Context> context);
    void HandleEndpointConnected(std::shared_ptr<Context> context);
};

} // namespace remote
} // namespace protoactor

#endif // PROTOACTOR_REMOTE_ENDPOINT_WATCHER_H
