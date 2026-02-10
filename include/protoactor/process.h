#ifndef PROTOACTOR_PROCESS_H
#define PROTOACTOR_PROCESS_H

#include "pid.h"
#include <memory>

namespace protoactor {

/**
 * @brief Process interface defines the base contract for interaction of actors.
 */
class Process {
public:
    virtual ~Process() = default;
    
    /**
     * @brief Send a user message to the process.
     * @param pid The PID (may be nullptr for some processes)
     * @param message The message to send
     */
    virtual void SendUserMessage(std::shared_ptr<PID> pid, std::shared_ptr<void> message) = 0;
    
    /**
     * @brief Send a system message to the process.
     * @param pid The PID (may be nullptr for some processes)
     * @param message The message to send
     */
    virtual void SendSystemMessage(std::shared_ptr<PID> pid, std::shared_ptr<void> message) = 0;
    
    /**
     * @brief Stop the process.
     * @param pid The PID to stop
     */
    virtual void Stop(std::shared_ptr<PID> pid) = 0;
};

} // namespace protoactor

#endif // PROTOACTOR_PROCESS_H
