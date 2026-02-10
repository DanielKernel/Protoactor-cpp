#include "protoactor/pidset.h"

namespace protoactor {

std::shared_ptr<PIDSet> PIDSet::New() {
    return std::shared_ptr<PIDSet>(new PIDSet());
}

void PIDSet::Add(std::shared_ptr<PID> pid) {
    if (pid) {
        pids_.insert(pid);
    }
}

void PIDSet::Remove(std::shared_ptr<PID> pid) {
    if (pid) {
        pids_.erase(pid);
    }
}

bool PIDSet::Contains(std::shared_ptr<PID> pid) const {
    if (!pid) {
        return false;
    }
    return pids_.find(pid) != pids_.end();
}

std::vector<std::shared_ptr<PID>> PIDSet::GetAll() const {
    std::vector<std::shared_ptr<PID>> result;
    result.reserve(pids_.size());
    for (const auto& pid : pids_) {
        result.push_back(pid);
    }
    return result;
}

size_t PIDSet::Len() const {
    return pids_.size();
}

void PIDSet::Clear() {
    pids_.clear();
}

bool PIDSet::Empty() const {
    return pids_.empty();
}

} // namespace protoactor
