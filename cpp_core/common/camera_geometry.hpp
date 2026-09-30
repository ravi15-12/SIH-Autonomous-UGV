#pragma once

#include "camera_model.hpp"
#include "camera_pose.hpp"

namespace ugv {

struct CameraGeometry {
    CameraModel model;
    CameraPose pose;

    bool isValid() const noexcept {
        return model.calibrated() && pose.isValid();
    }
};

}  // namespace ugv
