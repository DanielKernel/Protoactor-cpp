#include "protoactor/remote/serializer.h"
#include <stdexcept>

namespace protoactor {
namespace remote {

// Static members
std::vector<std::shared_ptr<Serializer>> SerializerRegistry::serializers_;
std::mutex SerializerRegistry::mutex_;
int32_t SerializerRegistry::default_serializer_id_ = 0;

int32_t SerializerRegistry::RegisterSerializer(std::shared_ptr<Serializer> serializer) {
    std::lock_guard<std::mutex> lock(mutex_);
    int32_t id = static_cast<int32_t>(serializers_.size());
    serializers_.push_back(serializer);
    if (id == 0) {
        default_serializer_id_ = 0;
    }
    return id;
}

std::shared_ptr<Serializer> SerializerRegistry::GetSerializer(int32_t serializer_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (serializer_id < 0 || serializer_id >= static_cast<int32_t>(serializers_.size())) {
        return nullptr;
    }
    return serializers_[serializer_id];
}

std::pair<std::vector<uint8_t>, std::string> SerializerRegistry::Serialize(
    std::shared_ptr<void> message,
    int32_t serializer_id) {
    
    int32_t id = serializer_id;
    if (id < 0) {
        id = default_serializer_id_;
    }
    
    auto serializer = GetSerializer(id);
    if (!serializer) {
        throw std::runtime_error("Serializer not found: " + std::to_string(id));
    }
    
    auto bytes = serializer->Serialize(message);
    auto type_name = serializer->GetTypeName(message);
    
    return {bytes, type_name};
}

std::shared_ptr<void> SerializerRegistry::Deserialize(
    const std::vector<uint8_t>& bytes,
    const std::string& type_name,
    int32_t serializer_id) {
    
    auto serializer = GetSerializer(serializer_id);
    if (!serializer) {
        throw std::runtime_error("Serializer not found: " + std::to_string(serializer_id));
    }
    
    return serializer->Deserialize(type_name, bytes);
}

} // namespace remote
} // namespace protoactor
