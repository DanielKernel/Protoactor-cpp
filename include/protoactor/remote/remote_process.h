#ifndef PROTOACTOR_REMOTE_REMOTE_PROCESS_H
#define PROTOACTOR_REMOTE_REMOTE_PROCESS_H

#include "../process.h"
#include "../pid.h"
#include <memory>

namespace protoactor {
namespace remote {

// Forward declaration
class Remote;

/**
 * @brief RemoteProcess represents a process on a remote node.
 */
class RemoteProcess : public Process {
public:
    RemoteProcess(
        std::shared_ptr<PID> pid,
        std::shared_ptr<Remote> remote);
    
    void SendUserMessage(std::shared_ptr<PID> pid, std::shared_ptr<void> message) override;
    void SendSystemMessage(std::shared_ptr<PID> pid, std::shared_ptr<void> message) override;
    void Stop(std::shared_ptr<PID> pid) override;

private:
    std::shared_ptr<PID> pid_;
    std::shared_ptr<Remote> remote_;
};

} // namespace remote
} // namespace protoactor

#endif // PROTOACTOR_REMOTE_REMOTE_PROCESS_H
