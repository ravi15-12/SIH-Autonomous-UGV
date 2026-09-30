#pragma once

#include <cstddef>
#include <cstdint>

namespace ugv {

#pragma pack(push, 1)

struct PoseFrameHeader {
    static constexpr std::uint32_t kMagic = 0x55475650;
    static constexpr std::uint16_t kVersion = 1;

    std::uint32_t magic = kMagic;
    std::uint16_t version = kVersion;
    std::uint16_t header_size = 48;

    std::uint64_t sequence = 0;
    std::uint64_t timestamp_ns = 0;

    double x_m = 0.0;
    double y_m = 0.0;
    double yaw_rad = 0.0;
};

#pragma pack(pop)

static_assert(
    sizeof(PoseFrameHeader) == 48,
    "PoseFrameHeader size must remain 48 bytes."
);

}  // namespace ugv