#ifndef PROTOACTOR_MAILBOX_H
#define PROTOACTOR_MAILBOX_H

#include "process.h"
#include "dispatcher.h"
#include <memory>
#include <atomic>

namespace protoactor {

// Forward declarations
class MessageInvoker;

// Forward declarations
class MessageInvoker;
class Dispatcher;

/**
 * @brief Mailbox interface for enqueueing messages.
 */
class Mailbox {
public:
    virtual ~Mailbox() = default;
    
    /**
     * @brief Post a user message to the mailbox.
     * @param message The message to post
     */
    virtual void PostUserMessage(std::shared_ptr<void> message) = 0;
    
    /**
     * @brief Post a system message to the mailbox.
     * @param message The message to post
     */
    virtual void PostSystemMessage(std::shared_ptr<void> message) = 0;
    
    /**
     * @brief Register handlers for processing messages.
     * @param invoker Message invoker (void* for now, will be properly typed later)
     * @param dispatcher Dispatcher for scheduling
     */
    virtual void RegisterHandlers(std::shared_ptr<void> invoker, std::shared_ptr<Dispatcher> dispatcher) = 0;
    
    /**
     * @brief Start the mailbox.
     */
    virtual void Start() = 0;
    
    /**
     * @brief Get the count of user messages in the mailbox.
     * @return Message count
     */
    virtual int UserMessageCount() const = 0;
};

/**
 * @brief Mailbox producer function type.
 */
using MailboxProducer = std::function<std::shared_ptr<Mailbox>()>;

/**
 * @brief Create an unbounded mailbox.
 * @return Mailbox producer
 */
MailboxProducer Unbounded();

/**
 * @brief Create a bounded mailbox.
 * @param size Maximum size
 * @return Mailbox producer
 */
MailboxProducer Bounded(int size);

} // namespace protoactor

#endif // PROTOACTOR_MAILBOX_H
