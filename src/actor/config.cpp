#include "external/config.h"

namespace protoactor {

Config::Config()
    : metrics_enabled(false),
      dead_letter_request_logging(true),
      dead_letter_throttle_count(100),
      dead_letter_throttle_interval(std::chrono::milliseconds(1000)) {
}

std::shared_ptr<Config> Config::Default() {
    return std::make_shared<Config>();
}

} // namespace protoactor
