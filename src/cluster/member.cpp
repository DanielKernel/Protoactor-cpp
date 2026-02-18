#include "internal/cluster/member.h"
#include <sstream>

namespace protoactor {
namespace cluster {

Member::Member()
    : port(0), alive(true) {
}

Member::Member(const std::string& id, const std::string& host, int port)
    : id(id), host(host), port(port), alive(true) {
    UpdateHeartbeat();
}

std::string Member::Address() const {
    std::stringstream ss;
    ss << host << ":" << port;
    return ss.str();
}

bool Member::IsAlive() const {
    return alive;
}

void Member::UpdateHeartbeat() {
    last_heartbeat = std::chrono::system_clock::now();
    alive = true;
}

} // namespace cluster
} // namespace protoactor
