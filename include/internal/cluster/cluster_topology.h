#ifndef PROTOACTOR_CLUSTER_CLUSTER_TOPOLOGY_H
#define PROTOACTOR_CLUSTER_CLUSTER_TOPOLOGY_H

#include "member.h"
#include "external/messages.h"
#include <vector>
#include <string>
#include <memory>

namespace protoactor {
namespace cluster {

/**
 * @brief ClusterTopology represents the current state of the cluster.
 */
struct ClusterTopology : public SystemMessage {
    std::vector<std::shared_ptr<Member>> members;
    std::vector<std::shared_ptr<Member>> left;
    std::vector<std::string> blocked;
};

} // namespace cluster
} // namespace protoactor

#endif // PROTOACTOR_CLUSTER_CLUSTER_TOPOLOGY_H
