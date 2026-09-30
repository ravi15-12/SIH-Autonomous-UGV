#pragma once

#include "pose_frame.hpp"

#include <cstddef>
#include <cstdint>

namespace ugv {

class PoseFrameReader {
public:
    bool decode(
        const std::uint8_t* frame,
        std::size_t frame_size,
        PoseFrame& output
    ) const noexcept;
};

}  // namespace ugv