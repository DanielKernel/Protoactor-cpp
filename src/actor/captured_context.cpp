#include "protoactor/internal/actor/captured_context.h"
#include "protoactor/context.h"

namespace protoactor {

CapturedContext::CapturedContext(
    std::shared_ptr<MessageEnvelope> envelope,
    std::shared_ptr<Context> ctx)
    : message_envelope(envelope), context(ctx) {
}

void CapturedContext::Receive() {
    if (!context || !message_envelope) {
        return;
    }
    
    // Capture current context state
    auto current = context->Capture();
    
    // Reprocess the captured message
    context->Apply(shared_from_this());
    
    // Restore current context state
    if (current) {
        current->Apply();
    }
}

void CapturedContext::Apply() {
    if (!context || !message_envelope) {
        return;
    }
    context->Apply(shared_from_this());
}

} // namespace protoactor
