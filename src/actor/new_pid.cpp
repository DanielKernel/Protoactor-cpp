#include "protoactor/new_pid.h"
#include "protoactor/pid.h"

namespace protoactor {

std::shared_ptr<PID> NewPID(const std::string& address, const std::string& id) {
    return std::make_shared<PID>(address, id);
}

} // namespace protoactor
