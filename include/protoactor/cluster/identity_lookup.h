#ifndef PROTOACTOR_CLUSTER_IDENTITY_LOOKUP_H
#define PROTOACTOR_CLUSTER_IDENTITY_LOOKUP_H

#include "../pid.h"
#include "cluster.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <chrono>

namespace protoactor {
namespace cluster {

// Forward declarations
class Cluster;

/**
 * @brief IdentityLookup provides virtual actor (grain) identity resolution.
 */
class IdentityLookup {
public:
    virtual ~IdentityLookup() = default;
    
    /**
     * @brief Setup the identity lookup.
     * @param cluster Cluster instance
     * @param kinds Actor kinds
     * @param is_client Whether this is a client-only node
     */
    virtual void Setup(
        std::shared_ptr<Cluster> cluster,
        const std::vector<std::string>& kinds,
        bool is_client) = 0;
    
    /**
     * @brief Get a PID for a virtual actor identity.
     * @param cluster Cluster instance
     * @param identity Identity string
     * @param kind Actor kind
     * @param timeout Timeout duration
     * @return PID and error code
     */
    virtual std::pair<std::shared_ptr<PID>, std::error_code> Get(
        std::shared_ptr<Cluster> cluster,
        const std::string& identity,
        const std::string& kind,
        std::chrono::milliseconds timeout) = 0;
    
    /**
     * @brief Remove a cached PID.
     * @param identity Identity string
     * @param kind Actor kind
     */
    virtual void RemoveCache(const std::string& identity, const std::string& kind) = 0;
    
    /**
     * @brief Shutdown the identity lookup.
     */
    virtual void Shutdown() = 0;
};

/**
 * @brief Default identity lookup implementation.
 */
class DefaultIdentityLookup : public IdentityLookup {
public:
    void Setup(
        std::shared_ptr<Cluster> cluster,
        const std::vector<std::string>& kinds,
        bool is_client) override;
    
    std::pair<std::shared_ptr<PID>, std::error_code> Get(
        std::shared_ptr<Cluster> cluster,
        const std::string& identity,
        const std::string& kind,
        std::chrono::milliseconds timeout) override;
    
    void RemoveCache(const std::string& identity, const std::string& kind) override;
    void Shutdown() override;

private:
    std::shared_ptr<Cluster> cluster_;
    std::vector<std::string> kinds_;
    bool is_client_;
};

} // namespace cluster
} // namespace protoactor

#endif // PROTOACTOR_CLUSTER_IDENTITY_LOOKUP_H
