#ifndef PROTOACTOR_ROUTER_ROUTER_GROUP_H
#define PROTOACTOR_ROUTER_ROUTER_GROUP_H

#include "external/pid.h"
#include "external/props.h"
#include "external/context.h"
#include <memory>
#include <vector>
#include <string>

namespace protoactor {
namespace router {

/**
 * @brief RouterActorGroup manages a group of routee actors.
 */
class RouterActorGroup {
public:
    /**
     * @brief Create a router actor group.
     * @param context Actor context
     * @param routee_props Props for routee actors
     * @param count Number of routees
     * @return Router actor group
     */
    static std::shared_ptr<RouterActorGroup> New(
        std::shared_ptr<Context> context,
        std::shared_ptr<Props> routee_props,
        int count);
    
    /**
     * @brief Get all routee PIDs.
     * @return Vector of routee PIDs
     */
    std::vector<std::shared_ptr<PID>> GetRoutees() const;
    
    /**
     * @brief Add a routee.
     * @param pid Routee PID
     */
    void AddRoutee(std::shared_ptr<PID> pid);
    
    /**
     * @brief Remove a routee.
     * @param pid Routee PID
     */
    void RemoveRoutee(std::shared_ptr<PID> pid);
    
    /**
     * @brief Stop all routees.
     */
    void Stop();

private:
    std::vector<std::shared_ptr<PID>> routees_;
    std::shared_ptr<Context> context_;
    
public:
    RouterActorGroup(std::shared_ptr<Context> context);
};

/**
 * @brief RouterActorPool manages a pool of routee actors.
 */
class RouterActorPool {
public:
    /**
     * @brief Create a router actor pool.
     * @param context Actor context
     * @param routee_props Props for routee actors
     * @param pool_size Pool size
     * @return Router actor pool
     */
    static std::shared_ptr<RouterActorPool> New(
        std::shared_ptr<Context> context,
        std::shared_ptr<Props> routee_props,
        int pool_size);
    
    /**
     * @brief Get all routee PIDs.
     * @return Vector of routee PIDs
     */
    std::vector<std::shared_ptr<PID>> GetRoutees() const;
    
    /**
     * @brief Stop the pool.
     */
    void Stop();

private:
    std::vector<std::shared_ptr<PID>> routees_;
    std::shared_ptr<Context> context_;
    
public:
    RouterActorPool(std::shared_ptr<Context> context);
};

} // namespace router
} // namespace protoactor

#endif // PROTOACTOR_ROUTER_ROUTER_GROUP_H
