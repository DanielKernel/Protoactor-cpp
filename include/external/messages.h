#ifndef PROTOACTOR_MESSAGES_H
#define PROTOACTOR_MESSAGES_H

#include "external/pid.h"
#include <memory>
#include <string>
#include <chrono>
#include <functional>
#include <any>
#include <system_error>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace protoactor {

// Forward declarations
class Context;
class ReadonlyMessageHeader;

/**
 * @brief System message base interface.
 */
class SystemMessage {
public:
    virtual ~SystemMessage() = default;
};

/**
 * @brief Auto-receive message marker interface.
 */
class AutoReceiveMessage {
public:
    virtual ~AutoReceiveMessage() = default;
};

/**
 * @brief Message envelope containing header, message, and sender.
 * Uses a magic number to identify itself when cast from void*.
 */
struct MessageEnvelope {
    // Magic number to identify MessageEnvelope when cast from void*
    static constexpr uint64_t MAGIC = 0x454E56454C4F5045ULL; // "ENVELOPE" in hex

    uint64_t magic = MAGIC;
    std::shared_ptr<ReadonlyMessageHeader> header;
    std::shared_ptr<void> message;
    std::shared_ptr<PID> sender;

    MessageEnvelope();
    MessageEnvelope(
        std::shared_ptr<ReadonlyMessageHeader> h,
        std::shared_ptr<void> m,
        std::shared_ptr<PID> s);

    std::string GetHeader(const std::string& key) const;
    void SetHeader(const std::string& key, const std::string& value);

    // Check if a void* pointer points to a valid MessageEnvelope
    static bool IsEnvelope(const std::shared_ptr<void>& ptr);
};

/**
 * @brief Readonly message header interface.
 */
class ReadonlyMessageHeader {
public:
    virtual ~ReadonlyMessageHeader() = default;
    virtual std::string Get(const std::string& key) const = 0;
    virtual bool Contains(const std::string& key) const = 0;
};

/**
 * @brief Mutable message header interface.
 */
class MessageHeader : public ReadonlyMessageHeader {
public:
    virtual ~MessageHeader() = default;
    virtual void Set(const std::string& key, const std::string& value) = 0;
    virtual std::vector<std::string> Keys() const = 0;
    virtual int Length() const = 0;
    virtual std::unordered_map<std::string, std::string> ToMap() const = 0;
};

/**
 * @brief Lifecycle messages.
 */
struct Started : public SystemMessage, public AutoReceiveMessage {
    Started() = default;
};

struct Stopping : public SystemMessage, public AutoReceiveMessage {
    Stopping() = default;
};

struct Stopped : public SystemMessage, public AutoReceiveMessage {
    Stopped() = default;
};

struct Restarting : public SystemMessage, public AutoReceiveMessage {
    Restarting() = default;
};

struct ReceiveTimeout : public SystemMessage {
    ReceiveTimeout() = default;
};

/**
 * @brief Control messages.
 */
struct Stop : public SystemMessage {
    Stop() = default;
};

struct PoisonPill : public SystemMessage {
    PoisonPill() = default;
};

struct Restart : public SystemMessage {
    Restart() = default;
};

struct Watch : public SystemMessage {
    std::shared_ptr<PID> watcher;
    
    explicit Watch(std::shared_ptr<PID> w) : watcher(w) {
    }
};

struct Unwatch : public SystemMessage {
    std::shared_ptr<PID> watcher;
    
    explicit Unwatch(std::shared_ptr<PID> w) : watcher(w) {
    }
};

struct Terminated : public SystemMessage {
    std::shared_ptr<PID> who;
    enum Reason {
        Stopped,
        AddressTerminated,
        Disassociated
    } reason;
    
    Terminated(std::shared_ptr<PID> w, Reason r) : who(w), reason(r) {
    }
};

struct Failure : public SystemMessage {
    std::shared_ptr<PID> who;
    std::shared_ptr<void> reason;
    std::shared_ptr<void> message;
    std::shared_ptr<PID> parent;
    
    Failure(std::shared_ptr<PID> w, std::shared_ptr<void> r, std::shared_ptr<PID> p, std::shared_ptr<void> m)
        : who(w), reason(r), parent(p), message(m) {
    }
};

/**
 * @brief Continuation message for ReenterAfter.
 */
struct Continuation : public SystemMessage {
    std::function<void(std::shared_ptr<void>, std::error_code)> continuation;
    std::shared_ptr<void> message;
    
    Continuation(
        std::function<void(std::shared_ptr<void>, std::error_code)> cont,
        std::shared_ptr<void> msg)
        : continuation(cont), message(msg) {
    }
};

/**
 * @brief Helper functions for message handling.
 */
std::shared_ptr<MessageEnvelope> WrapEnvelope(std::shared_ptr<void> message_or_envelope);
std::tuple<std::shared_ptr<ReadonlyMessageHeader>, std::shared_ptr<void>, std::shared_ptr<PID>>
UnwrapEnvelope(std::shared_ptr<void> message_or_envelope);

} // namespace protoactor

#endif // PROTOACTOR_MESSAGES_H
