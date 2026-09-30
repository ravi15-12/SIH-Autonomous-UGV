#pragma once

#include "../common/perception_frame.hpp"

#include <cstddef>
#include <cstdint>

namespace ugv {

class PythonPerceptionReader {
public:
    static constexpr std::uint32_t kWidth = 512;
    static constexpr std::uint32_t kHeight = 512;
    static constexpr std::size_t kPixelCount =
        static_cast<std::size_t>(kWidth) * kHeight;

    static constexpr std::size_t kHeaderSize = 36;

    static constexpr std::size_t kSegmentationSize =
        kPixelCount;

    static constexpr std::size_t kFloatArraySize =
        kPixelCount * sizeof(float);

    static constexpr std::size_t kPayloadSize =
        kSegmentationSize +
        (3 * kFloatArraySize);

    static constexpr std::size_t kFrameSize =
        kHeaderSize + kPayloadSize;

    bool decode(
        const std::uint8_t* frame,
        std::size_t frame_size,
        PerceptionFrame& output) const noexcept;
};

}  // namespace ugv