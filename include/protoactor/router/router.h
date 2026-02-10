#ifndef PROTOACTOR_ROUTER_ROUTER_H
#define PROTOACTOR_ROUTER_ROUTER_H

#include "../actor.h"
#include "../context.h"
#include "../pid.h"
#include <memory>
#include <vector>

namespace protoactor {
namespace router {

// Forward declarations
class RouterState;

// SenderContext is a shared_ptr to protoactor::Context
using SenderContext = std::shared_ptr<protoactor::Context>;

/**
 * @brief Router interface for routing messages to multiple routees.
 */
class Router {
public:
    virtual ~Router() = default;
    
    /**
     * @brief Route a message to one or more routees.
     * @param message The message to route
     */
    virtual void RouteMessage(std::shared_ptr<void> message) = 0;
    
    /**
     * @brief Set the routees (target PIDs).
     * @param routees Vector of routee PIDs
     */
    virtual void SetRoutees(const std::vector<std::shared_ptr<PID>>& routees) = 0;
    
    /**
     * @brief Get the routees.
     * @return Vector of routee PIDs
     */
    virtual std::vector<std::shared_ptr<PID>> GetRoutees() const = 0;
    
    /**
     * @brief Set the sender context.
     * @param sender Sender context (Context shared_ptr)
     */
    virtual void SetSender(SenderContext sender) = 0;
};

/**
 * @brief Broadcast router sends messages to all routees.
 */
class BroadcastRouter : public Router {
public:
    void RouteMessage(std::shared_ptr<void> message) override;
    void SetRoutees(const std::vector<std::shared_ptr<PID>>& routees) override;
    std::vector<std::shared_ptr<PID>> GetRoutees() const override;
    void SetSender(SenderContext sender) override;

protected:
    std::vector<std::shared_ptr<PID>> routees_;
    SenderContext sender_;
};

/**
 * @brief Round-robin router distributes messages in round-robin fashion.
 */
class RoundRobinRouter : public Router {
public:
    RoundRobinRouter();
    void RouteMessage(std::shared_ptr<void> message) override;
    void SetRoutees(const std::vector<std::shared_ptr<PID>>& routees) override;
    std::vector<std::shared_ptr<PID>> GetRoutees() const override;
    void SetSender(SenderContext sender) override;

protected:
    std::vector<std::shared_ptr<PID>> routees_;
    SenderContext sender_;
    std::atomic<int> current_index_;
};

/**
 * @brief Random router distributes messages randomly.
 */
class RandomRouter : public Router {
public:
    void RouteMessage(std::shared_ptr<void> message) override;
    void SetRoutees(const std::vector<std::shared_ptr<PID>>& routees) override;
    std::vector<std::shared_ptr<PID>> GetRoutees() const override;
    void SetSender(SenderContext sender) override;

protected:
    std::vector<std::shared_ptr<PID>> routees_;
    SenderContext sender_;
};

/**
 * @brief Consistent hash router uses consistent hashing.
 */
class ConsistentHashRouter : public Router {
public:
    void RouteMessage(std::shared_ptr<void> message) override;
    void SetRoutees(const std::vector<std::shared_ptr<PID>>& routees) override;
    std::vector<std::shared_ptr<PID>> GetRoutees() const override;
    void SetSender(SenderContext sender) override;

protected:
    std::vector<std::shared_ptr<PID>> routees_;
    SenderContext sender_;
    // Hash ring implementation would go here
};

} // namespace router
} // namespace protoactor

#endif // PROTOACTOR_ROUTER_ROUTER_H
