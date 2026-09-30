#include "pose_frame_reader.hpp"

#include "pose_frame_header.hpp"

#include <cstring>

namespace ugv {

bool PoseFrameReader::decode(
    const std::uint8_t* frame,
    std::size_t frame_size,
    PoseFrame& output
) const noexcept {
    output = {};

    if (frame == nullptr ||
        frame_size < sizeof(PoseFrameHeader)) {
        return false;
    }

    PoseFrameHeader header{};

    std::memcpy(
        &header,
        frame,
        sizeof(PoseFrameHeader)
    );

    if (header.magic != PoseFrameHeader::kMagic ||
        header.version != PoseFrameHeader::kVersion ||
        header.header_size != sizeof(PoseFrameHeader)) {
        return false;
    }

    output.sequence = header.sequence;
    output.timestamp_ns = header.timestamp_ns;
    output.x_m = header.x_m;
    output.y_m = header.y_m;
    output.yaw_rad = header.yaw_rad;

    return output.isValid();
}

}  // namespace ugv