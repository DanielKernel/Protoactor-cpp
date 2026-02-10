#include "protoactor/stream/stream.h"
#include "protoactor/actor.h"
#include "protoactor/props.h"
#include "protoactor/messages.h"
#include <queue>
#include <mutex>
#include <condition_variable>

namespace protoactor {
namespace stream {

// Internal stream actor
template<typename T>
class StreamActor : public Actor {
public:
    StreamActor(std::shared_ptr<std::queue<T>> queue,
                std::shared_ptr<std::mutex> mutex,
                std::shared_ptr<std::condition_variable> cv)
        : queue_(queue), mutex_(mutex), cv_(cv) {
    }
    
    void Receive(std::shared_ptr<Context> context) override {
        auto msg = context->Message();
        if (!msg) {
            return;
        }
        
        // Note: We cannot use dynamic_pointer_cast on std::shared_ptr<void>
        // For template types, we use static_cast assuming the caller knows the type
        // In full implementation, we would use type information or a message registry
        auto typed_msg = std::static_pointer_cast<T>(msg);
        if (typed_msg) {
            std::lock_guard<std::mutex> lock(*mutex_);
            queue_->push(*typed_msg);
            cv_->notify_one();
        }
    }

private:
    std::shared_ptr<std::queue<T>> queue_;
    std::shared_ptr<std::mutex> mutex_;
    std::shared_ptr<std::condition_variable> cv_;
};

// Subscription implementation
class SubscriptionImpl : public Subscription {
public:
    SubscriptionImpl(std::shared_ptr<PID> pid, std::shared_ptr<Context> context)
        : pid_(pid), context_(context) {
    }
    
    void Unsubscribe() override {
        if (pid_ && context_) {
            context_->Stop(pid_);
        }
    }

private:
    std::shared_ptr<PID> pid_;
    std::shared_ptr<Context> context_;
};

// TypedStream implementation
template<typename T>
TypedStream<T>::TypedStream(std::shared_ptr<PID> pid, std::shared_ptr<Context> context)
    : stream_pid_(pid), context_(context) {
}

template<typename T>
std::shared_ptr<PID> TypedStream<T>::GetPID() const {
    return stream_pid_;
}

template<typename T>
std::shared_ptr<Subscription> TypedStream<T>::Subscribe(MessageHandler handler) {
    // In a full implementation, this would set up message forwarding
    // For now, return a placeholder subscription
    return std::make_shared<SubscriptionImpl>(stream_pid_, context_);
}

template<typename T>
void TypedStream<T>::Publish(const T& message) {
    if (stream_pid_ && context_) {
        context_->Send(stream_pid_, std::make_shared<T>(message));
    }
}

// UntypedStream implementation
UntypedStream::UntypedStream(std::shared_ptr<PID> pid, std::shared_ptr<Context> context)
    : stream_pid_(pid), context_(context) {
}

std::shared_ptr<PID> UntypedStream::GetPID() const {
    return stream_pid_;
}

std::shared_ptr<Subscription> UntypedStream::Subscribe(MessageHandler handler) {
    return std::make_shared<SubscriptionImpl>(stream_pid_, context_);
}

void UntypedStream::Publish(std::shared_ptr<void> message) {
    if (stream_pid_ && context_) {
        context_->Send(stream_pid_, message);
    }
}

// Template implementation moved to header file

// Template implementation moved to header file

} // namespace stream
} // namespace protoactor
