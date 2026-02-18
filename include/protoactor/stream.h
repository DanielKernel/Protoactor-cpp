#ifndef PROTOACTOR_STREAM_STREAM_H
#define PROTOACTOR_STREAM_STREAM_H

#include "../pid.h"
#include "../context.h"
#include <memory>
#include <functional>
#include <any>

namespace protoactor {
namespace stream {

/**
 * @brief Stream subscription handle.
 */
class Subscription {
public:
    virtual ~Subscription() = default;
    virtual void Unsubscribe() = 0;
};

/**
 * @brief Typed stream for type-safe message streaming.
 */
template<typename T>
class TypedStream {
public:
    using MessageHandler = std::function<void(T)>;
    
    /**
     * @brief Subscribe to the stream.
     * @param handler Message handler
     * @return Subscription handle
     */
    std::shared_ptr<Subscription> Subscribe(MessageHandler handler);
    
    /**
     * @brief Publish a message to the stream.
     * @param message Message to publish
     */
    void Publish(const T& message);
    
    /**
     * @brief Get the PID of the stream actor.
     * @return Stream PID
     */
    std::shared_ptr<PID> GetPID() const;

private:
    std::shared_ptr<PID> stream_pid_;
    std::shared_ptr<Context> context_;
    
    TypedStream(std::shared_ptr<PID> pid, std::shared_ptr<Context> context);
    friend class Stream;
};

/**
 * @brief Untyped stream for dynamic message streaming.
 */
class UntypedStream {
public:
    using MessageHandler = std::function<void(std::shared_ptr<void>)>;
    
    /**
     * @brief Subscribe to the stream.
     * @param handler Message handler
     * @return Subscription handle
     */
    std::shared_ptr<Subscription> Subscribe(MessageHandler handler);
    
    /**
     * @brief Publish a message to the stream.
     * @param message Message to publish
     */
    void Publish(std::shared_ptr<void> message);
    
    /**
     * @brief Get the PID of the stream actor.
     * @return Stream PID
     */
    std::shared_ptr<PID> GetPID() const;

private:
    std::shared_ptr<PID> stream_pid_;
    std::shared_ptr<Context> context_;
    
    UntypedStream(std::shared_ptr<PID> pid, std::shared_ptr<Context> context);
    friend class Stream;
};

/**
 * @brief Stream factory and utilities.
 */
class Stream {
public:
    /**
     * @brief Create a typed stream.
     * @param context Actor context
     * @return Typed stream
     */
    template<typename T>
    static TypedStream<T> Of(std::shared_ptr<Context> context);
    
    /**
     * @brief Create an untyped stream.
     * @param context Actor context
     * @return Untyped stream
     */
    static UntypedStream OfUntyped(std::shared_ptr<Context> context);
};

} // namespace stream
} // namespace protoactor

#endif // PROTOACTOR_STREAM_STREAM_H
