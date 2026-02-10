#ifndef PROTOACTOR_MESSAGE_BATCH_H
#define PROTOACTOR_MESSAGE_BATCH_H

#include <vector>
#include <memory>

namespace protoactor {

/**
 * @brief MessageBatch contains multiple messages to be processed together.
 */
class MessageBatch {
public:
    /**
     * @brief Get all messages in the batch.
     * @return Vector of messages
     */
    virtual std::vector<std::shared_ptr<void>> GetMessages() const = 0;
    
    virtual ~MessageBatch() = default;
};

} // namespace protoactor

#endif // PROTOACTOR_MESSAGE_BATCH_H
