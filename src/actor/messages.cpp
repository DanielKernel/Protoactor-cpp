#include "external/messages.h"
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
    : magic(MAGIC), header(nullptr), message(nullptr), sender(nullptr) {
}

MessageEnvelope::MessageEnvelope(
    std::shared_ptr<ReadonlyMessageHeader> h,
    std::shared_ptr<void> msg,
    std::shared_ptr<PID> snd)
    : magic(MAGIC), header(h), message(msg), sender(snd) {
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

bool MessageEnvelope::IsEnvelope(const std::shared_ptr<void>& ptr) {
    if (!ptr) {
        return false;
    }
    // Use static_pointer_cast to access the magic field
    // This is safe because we're only reading the first field
    auto envelope = std::static_pointer_cast<MessageEnvelope>(ptr);
    return envelope->magic == MAGIC;
}

// Helper functions
std::shared_ptr<MessageEnvelope> WrapEnvelope(std::shared_ptr<void> message) {
    if (!message) {
        return nullptr;
    }

    // Check if it's already an envelope using the magic number
    if (MessageEnvelope::IsEnvelope(message)) {
        return std::static_pointer_cast<MessageEnvelope>(message);
    }

    // Otherwise, wrap it in a new envelope
    return std::make_shared<MessageEnvelope>(nullptr, message, nullptr);
}

std::tuple<std::shared_ptr<ReadonlyMessageHeader>, std::shared_ptr<void>, std::shared_ptr<PID>>
UnwrapEnvelope(std::shared_ptr<void> message) {
    if (!message) {
        return std::make_tuple(nullptr, nullptr, nullptr);
    }

    // Check if it's an envelope using the magic number
    if (MessageEnvelope::IsEnvelope(message)) {
        auto envelope = std::static_pointer_cast<MessageEnvelope>(message);
        return std::make_tuple(envelope->header, envelope->message, envelope->sender);
    }

    // Not an envelope, assume it's a raw message
    return std::make_tuple(nullptr, message, nullptr);
}

std::shared_ptr<ReadonlyMessageHeader> EmptyMessageHeader() {
    return std::make_shared<MessageHeaderImpl>();
}

} // namespace protoactor
