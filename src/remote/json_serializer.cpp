#include "protoactor/internal/remote/serializer.h"
#include <stdexcept>
#include <sstream>

#ifdef PROTOACTOR_USE_JSON
    #include <nlohmann/json.hpp>
    using json = nlohmann::json;
#endif

namespace protoactor {
namespace remote {

/**
 * @brief JSON serializer implementation.
 * 
 * Uses nlohmann/json if available, otherwise provides placeholder.
 */
class JSONSerializer : public Serializer {
public:
    std::vector<uint8_t> Serialize(const std::any& message) override {
#ifdef PROTOACTOR_USE_JSON
        // TODO: When JSON library is integrated:
        // 1. Convert message to JSON
        // 2. Serialize to bytes
        
        // Placeholder
        return std::vector<uint8_t>();
#else
        throw std::runtime_error("JSON serializer requires nlohmann/json library");
#endif
    }
    
    std::any Deserialize(const std::string& type_name, const std::vector<uint8_t>& bytes) override {
#ifdef PROTOACTOR_USE_JSON
        // TODO: When JSON library is integrated:
        // 1. Parse JSON from bytes
        // 2. Convert to message type
        
        throw std::runtime_error("JSONSerializer::Deserialize not yet implemented");
#else
        throw std::runtime_error("JSON serializer requires nlohmann/json library");
#endif
    }
    
    std::string GetTypeName(const std::any& message) override {
        // Return typeid name as fallback
        return std::string(message.type().name());
    }
    
    int32_t GetSerializerID() const override {
        return 1; // JSON serializer ID
    }
};

// Register JSON serializer on initialization (if JSON is enabled)
#ifdef PROTOACTOR_USE_JSON
namespace {
    struct JSONSerializerRegistrar {
        JSONSerializerRegistrar() {
            auto serializer = std::make_shared<JSONSerializer>();
            SerializerRegistry::RegisterSerializer(serializer);
        }
    };
    
    static JSONSerializerRegistrar registrar;
}
#endif

} // namespace remote
} // namespace protoactor
