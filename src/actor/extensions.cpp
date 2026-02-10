#include "protoactor/extensions.h"
#include <mutex>

namespace protoactor {

ExtensionID Extensions::next_id_ = 1;

std::shared_ptr<Extensions> Extensions::New() {
    return std::make_shared<Extensions>();
}

ExtensionID Extensions::NextExtensionID() {
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    return next_id_++;
}

void Extensions::Register(ExtensionID id, std::shared_ptr<void> extension) {
    extensions_[id] = extension;
}

std::shared_ptr<void> Extensions::Get(ExtensionID id) const {
    auto it = extensions_.find(id);
    if (it != extensions_.end()) {
        return it->second;
    }
    return nullptr;
}

} // namespace protoactor
