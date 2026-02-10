#include "protoactor/platform.h"
#include <thread>
#include <unistd.h>
#ifdef __linux__
#include <sys/sysinfo.h>
#endif

namespace protoactor {
namespace platform {

int GetCPUCount() {
    // Linux: use sysconf
    long count = sysconf(_SC_NPROCESSORS_ONLN);
    if (count > 0) {
        return static_cast<int>(count);
    }
    // Fallback to C++11
    return static_cast<int>(std::thread::hardware_concurrency());
}

} // namespace platform
} // namespace protoactor
