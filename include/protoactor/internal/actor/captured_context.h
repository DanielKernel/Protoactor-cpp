#ifndef PROTOACTOR_CAPTURED_CONTEXT_H
#define PROTOACTOR_CAPTURED_CONTEXT_H

#include "protoactor/messages.h"
#include "protoactor/context.h"
#include <memory>

namespace protoactor {

/**
 * @brief CapturedContext stores a message envelope and context for later reprocessing.
 */
class CapturedContext : public std::enable_shared_from_this<CapturedContext> {
public:
    /**
     * @brief Create a captured context.
     * @param envelope Message envelope
     * @param ctx Context
     */
    CapturedContext(
        std::shared_ptr<MessageEnvelope> envelope,
        std::shared_ptr<Context> ctx);
    
    /**
     * @brief Reprocess the captured message on the captured context.
     * Captures the current context state before processing and restores it afterwards.
     */
    void Receive();
    
    /**
     * @brief Apply restores the stored message to the actor context.
     */
    void Apply();
    
    std::shared_ptr<MessageEnvelope> message_envelope;
    std::shared_ptr<Context> context;
};

} // namespace protoactor

#endif // PROTOACTOR_CAPTURED_CONTEXT_H
