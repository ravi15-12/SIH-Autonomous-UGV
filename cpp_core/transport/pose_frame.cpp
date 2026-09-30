#include "pose_frame.hpp"

#include <cmath>

namespace ugv {

bool PoseFrame::isValid() const noexcept {
    return sequence > 0 &&
           timestamp_ns > 0 &&
           std::isfinite(x_m) &&
           std::isfinite(y_m) &&
           std::isfinite(yaw_rad);
}

}  // namespace ugv