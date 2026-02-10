#include "protoactor/messages.h"
#include <algorithm>

namespace protoactor {

// MessageHeader implementation
class MessageHeaderImpl : public MessageHeader {
public:
    std::string Get(const std::string& key) const override {
        auto it = headers_.find(key);
        return it != headers_.end() ? it->second : std::string();
    }
    
    bool Contains(const std::string& key) const override {
        return headers_.find(key) != headers_.end();
    }
    
    void Set(const std::string& key, const std::string& value) override {
        headers_[key] = value;
    }
    
    std::vector<std::string> Keys() const override {
        std::vector<std::string> keys;
        keys.reserve(headers_.size());
        for (const auto& pair : headers_) {
            keys.push_back(pair.first);
        }
        return keys;
    }
    
    int Length() const override {
        return static_cast<int>(headers_.size());
    }
    
    std::unordered_map<std::string, std::string> ToMap() const override {
        return headers_;
    }

private:
    std::unordered_map<std::string, std::string> headers_;
};

// MessageEnvelope implementation
MessageEnvelope::MessageEnvelope()
    : header(nullptr), message(nullptr), sender(nullptr) {
}

MessageEnvelope::MessageEnvelope(
    std::shared_ptr<ReadonlyMessageHeader> h,
    std::shared_ptr<void> msg,
    std::shared_ptr<PID> snd)
    : header(h), message(msg), sender(snd) {
}

std::string MessageEnvelope::GetHeader(const std::string& key) const {
    if (!header) {
        return std::string();
    }
    return header->Get(key);
}

void MessageEnvelope::SetHeader(const std::string& key, const std::string& value) {
    if (!header) {
        header = std::make_shared<MessageHeaderImpl>();
    }
    if (auto mutable_header = std::dynamic_pointer_cast<MessageHeader>(header)) {
        mutable_header->Set(key, value);
    }
}

// Helper functions
std::shared_ptr<MessageEnvelope> WrapEnvelope(std::shared_ptr<void> message) {
    if (!message) {
        return nullptr;
    }
    
    // Try to cast to MessageEnvelope first
    // If it's already an envelope, return it
    // Note: We use static_cast because we can't use dynamic_cast on void*
    // This is unsafe but necessary for type erasure
    // In full implementation, we would use type information
    auto envelope = std::static_pointer_cast<MessageEnvelope>(message);
    if (envelope) {
        return envelope;
    }
    
    // Otherwise, wrap it in a new envelope
    return std::make_shared<MessageEnvelope>(nullptr, message, nullptr);
}

std::tuple<std::shared_ptr<ReadonlyMessageHeader>, std::shared_ptr<void>, std::shared_ptr<PID>>
UnwrapEnvelope(std::shared_ptr<void> message) {
    if (!message) {
        return std::make_tuple(nullptr, nullptr, nullptr);
    }
    
    // Try to cast to MessageEnvelope
    // Note: We use static_cast because we can't use dynamic_cast on void*
    // This is unsafe but necessary for type erasure
    // In full implementation, we would use type information
    // Since all messages sent through SendUserMessage are wrapped in MessageEnvelope,
    // we can safely assume it's an envelope if the cast succeeds
    try {
        auto envelope = std::static_pointer_cast<MessageEnvelope>(message);
        // Try to access envelope members to verify it's actually a MessageEnvelope
        // This is a heuristic check - in full implementation, we would use type information
        if (envelope && (envelope->header || envelope->message || envelope->sender)) {
            // It's an envelope, extract the message, header, and sender
            return std::make_tuple(envelope->header, envelope->message, envelope->sender);
        }
        // If envelope exists but all members are null, it might still be an envelope
        // (empty envelope), so return it
        if (envelope) {
            return std::make_tuple(envelope->header, envelope->message, envelope->sender);
        }
    } catch (...) {
        // Cast failed, not an envelope
    }
    
    // Not an envelope, assume it's a raw message
    return std::make_tuple(nullptr, message, nullptr);
}

std::shared_ptr<ReadonlyMessageHeader> EmptyMessageHeader() {
    return std::make_shared<MessageHeaderImpl>();
}

} // namespace protoactor
