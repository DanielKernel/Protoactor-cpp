#ifndef PROTOACTOR_NEW_PID_H
#define PROTOACTOR_NEW_PID_H

#include "protoactor/pid.h"
#include <memory>
#include <string>

namespace protoactor {

/**
 * @brief Create a new PID with the given address and id.
 * @param address The network address
 * @param id The unique identifier
 * @return A new PID instance
 */
std::shared_ptr<PID> NewPID(const std::string& address, const std::string& id);

} // namespace protoactor

#endif // PROTOACTOR_NEW_PID_H
