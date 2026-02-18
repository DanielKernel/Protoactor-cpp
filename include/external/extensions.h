#ifndef PROTOACTOR_EXTENSIONS_H
#define PROTOACTOR_EXTENSIONS_H

#include <memory>
#include <unordered_map>
#include <cstdint>

namespace protoactor {

using ExtensionID = uint32_t;

/**
 * @brief Extensions manages system extensions.
 */
class Extensions {
public:
    /**
     * @brief Create new extensions.
     * @return Extensions instance
     */
    static std::shared_ptr<Extensions> New();
    
    /**
     * @brief Get next extension ID.
     * @return Next ID
     */
    static ExtensionID NextExtensionID();
    
    /**
     * @brief Register an extension.
     * @param id Extension ID
     * @param extension Extension instance
     */
    void Register(ExtensionID id, std::shared_ptr<void> extension);
    
    /**
     * @brief Get an extension.
     * @param id Extension ID
     * @return Extension or nullptr
     */
    std::shared_ptr<void> Get(ExtensionID id) const;

private:
    std::unordered_map<ExtensionID, std::shared_ptr<void>> extensions_;
    static ExtensionID next_id_;
    
public:
    Extensions() = default;
};

} // namespace protoactor

#endif // PROTOACTOR_EXTENSIONS_H
