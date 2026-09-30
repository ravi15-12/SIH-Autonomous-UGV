#pragma once

#include <cmath>

namespace ugv {

struct CameraModel {
    int image_width = 0;
    int image_height = 0;

    // Pinhole intrinsic parameters.
    // These must come from actual camera calibration
    // before metric geometry is enabled.
    double fx = 0.0;
    double fy = 0.0;
    double cx = 0.0;
    double cy = 0.0;

    bool calibrated() const noexcept {
        return image_width > 0 &&
               image_height > 0 &&
               std::isfinite(fx) &&
               std::isfinite(fy) &&
               std::isfinite(cx) &&
               std::isfinite(cy) &&
               fx > 0.0 &&
               fy > 0.0 &&
               cx >= 0.0 &&
               cx < image_width &&
               cy >= 0.0 &&
               cy < image_height;
    }
};

}  // namespace ugv
