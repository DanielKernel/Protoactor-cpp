#include "internal/remote/blocklist.h"
#include <algorithm>
#include <memory>
#ifdef __cpp_lib_shared_mutex
using std::shared_lock;
#else
using std::lock_guard;
#define shared_lock lock_guard
#endif

namespace protoactor {
namespace remote {

std::shared_ptr<BlockList> BlockList::New() {
    return std::shared_ptr<BlockList>(new BlockList());
}

void BlockList::Block(const std::vector<std::string>& member_ids) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    for (const auto& id : member_ids) {
        blocked_members_.insert(id);
    }
}

void BlockList::Block(const std::string& member_id) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    blocked_members_.insert(member_id);
}

bool BlockList::IsBlocked(const std::string& member_id) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return blocked_members_.find(member_id) != blocked_members_.end();
}

std::vector<std::string> BlockList::BlockedMembers() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    std::vector<std::string> result;
    result.reserve(blocked_members_.size());
    for (const auto& id : blocked_members_) {
        result.push_back(id);
    }
    return result;
}

size_t BlockList::Len() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return blocked_members_.size();
}

} // namespace remote
} // namespace protoactor
