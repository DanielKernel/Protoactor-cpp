#ifndef PROTOACTOR_REMOTE_SERIALIZER_H
#define PROTOACTOR_REMOTE_SERIALIZER_H

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <any>
#include <cstdint>

namespace protoactor {
namespace remote {

/**
 * @brief Serializer interface for encoding and decoding messages.
 */
class Serializer {
public:
    virtual ~Serializer() = default;
    
    /**
     * @brief Serialize a message to bytes.
     * @param message The message to serialize
     * @return Serialized bytes
     */
    virtual std::vector<uint8_t> Serialize(std::shared_ptr<void> message) = 0;
    
    /**
     * @brief Deserialize bytes to a message.
     * @param type_name The type name of the message
     * @param bytes The serialized bytes
     * @return Deserialized message
     */
    virtual std::shared_ptr<void> Deserialize(const std::string& type_name, const std::vector<uint8_t>& bytes) = 0;
    
    /**
     * @brief Get the type name of a message.
     * @param message The message
     * @return Type name
     */
    virtual std::string GetTypeName(std::shared_ptr<void> message) = 0;
    
    /**
     * @brief Get the serializer ID.
     * @return Serializer ID
     */
    virtual int32_t GetSerializerID() const = 0;
};

/**
 * @brief Serializer registry for managing serializers.
 */
class SerializerRegistry {
public:
    /**
     * @brief Register a serializer.
     * @param serializer The serializer to register
     * @return Serializer ID
     */
    static int32_t RegisterSerializer(std::shared_ptr<Serializer> serializer);
    
    /**
     * @brief Get a serializer by ID.
     * @param serializer_id Serializer ID
     * @return Serializer or nullptr if not found
     */
    static std::shared_ptr<Serializer> GetSerializer(int32_t serializer_id);
    
    /**
     * @brief Serialize a message.
     * @param message The message to serialize
     * @param serializer_id Serializer ID (use default if < 0)
     * @return Pair of (serialized bytes, type name)
     */
    static std::pair<std::vector<uint8_t>, std::string> Serialize(
        std::shared_ptr<void> message, 
        int32_t serializer_id = -1);
    
    /**
     * @brief Deserialize bytes to a message.
     * @param bytes Serialized bytes
     * @param type_name Type name
     * @param serializer_id Serializer ID
     * @return Deserialized message
     */
    static std::shared_ptr<void> Deserialize(
        const std::vector<uint8_t>& bytes,
        const std::string& type_name,
        int32_t serializer_id);

private:
    static std::vector<std::shared_ptr<Serializer>> serializers_;
    static std::mutex mutex_;
    static int32_t default_serializer_id_;
};

} // namespace remote
} // namespace protoactor

#endif // PROTOACTOR_REMOTE_SERIALIZER_H
