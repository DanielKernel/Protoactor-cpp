#ifndef PROTOACTOR_DEDUPLICATION_CONTEXT_H
#define PROTOACTOR_DEDUPLICATION_CONTEXT_H

#include "protoactor/props.h"
#include "protoactor/messages.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <chrono>
#include <functional>

namespace protoactor {

/**
 * @brief Deduplicator extracts a deduplication key from a message.
 * 
 * Messages yielding the same key within the TTL are treated as duplicates.
 */
using Deduplicator = std::function<std::string(std::shared_ptr<void>)>;

/**
 * @brief DeduplicationContext returns a receiver middleware that ignores duplicate messages
 * as determined by the provided Deduplicator function. Keys expire after the
 * given TTL, allowing messages to be processed again once the entry is purged.
 * 
 * @param fn Deduplicator function that extracts a key from a message
 * @param ttl Time-to-live for deduplication keys
 * @return ReceiverMiddleware function
 */
ReceiverMiddleware DeduplicationContext(Deduplicator fn, std::chrono::milliseconds ttl);

} // namespace protoactor

#endif // PROTOACTOR_DEDUPLICATION_CONTEXT_H
