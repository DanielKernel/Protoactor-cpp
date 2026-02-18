#ifndef PROTOACTOR_REMOTE_ENDPOINT_MANAGER_H
#define PROTOACTOR_REMOTE_ENDPOINT_MANAGER_H

#include "protoactor/pid.h"
#include "protoactor/messages.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <any>

namespace protoactor {
namespace remote {

// Forward declarations
class Remote;
class EndpointWriter;
class EndpointWatcher;

/**
 * @brief Endpoint represents a connection to a remote address.
 */
struct Endpoint {
    std::shared_ptr<PID> writer;
    std::shared_ptr<PID> watcher;
    
    std::string Address() const;
};

/**
 * @brief EndpointManager manages connections to remote endpoints.
 */
class EndpointManager {
public:
    explicit EndpointManager(std::shared_ptr<Remote> remote);
    ~EndpointManager();
    
    /**
     * @brief Start the endpoint manager.
     */
    void Start();
    
    /**
     * @brief Stop the endpoint manager.
     */
    void Stop();
    
    /**
     * @brief Ensure a connection to the given address.
     * @param address Remote address
     * @return Endpoint or nullptr if connection failed
     */
    std::shared_ptr<Endpoint> EnsureConnected(const std::string& address);
    
    /**
     * @brief Remove an endpoint.
     * @param address Remote address
     */
    void RemoveEndpoint(const std::string& address);
    
    /**
     * @brief Handle remote message delivery.
     * @param target Target PID
     * @param message Message to deliver
     * @param sender Sender PID
     */
    void RemoteDeliver(
        std::shared_ptr<PID> target,
        std::shared_ptr<void> message,
        std::shared_ptr<PID> sender);
    
    /**
     * @brief Handle remote watch.
     * @param watcher Watcher PID
     * @param watchee Watchee PID
     */
    void RemoteWatch(
        std::shared_ptr<PID> watcher,
        std::shared_ptr<PID> watchee);
    
    /**
     * @brief Handle remote unwatch.
     * @param watcher Watcher PID
     * @param watchee Watchee PID
     */
    void RemoteUnwatch(
        std::shared_ptr<PID> watcher,
        std::shared_ptr<PID> watchee);
    
    /**
     * @brief Handle remote terminate.
     * @param watcher Watcher PID
     * @param watchee Watchee PID
     */
    void RemoteTerminate(
        std::shared_ptr<PID> watcher,
        std::shared_ptr<PID> watchee);

private:
    std::shared_ptr<Remote> remote_;
    std::unordered_map<std::string, std::shared_ptr<Endpoint>> connections_;
    std::mutex connections_mutex_;
    std::atomic<bool> stopped_;
    std::shared_ptr<PID> endpoint_supervisor_;
    std::shared_ptr<PID> activator_;
    std::shared_ptr<void> endpoint_sub_; // Event subscription (std::shared_ptr<Subscription>)
    
    void StartActivator();
    void StopActivator();
    void StartSupervisor();
    void StopSupervisor();
    void HandleEndpointEvent(std::shared_ptr<void> event);
};

} // namespace remote
} // namespace protoactor

#endif // PROTOACTOR_REMOTE_ENDPOINT_MANAGER_H
