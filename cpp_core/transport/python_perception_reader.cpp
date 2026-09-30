#include "python_perception_reader.hpp"

#include "../common/perception_frame_header.hpp"

#include <cmath>
#include <cstring>

namespace ugv {

bool PythonPerceptionReader::decode(
    const std::uint8_t* frame,
    std::size_t frame_size,
    PerceptionFrame& output) const noexcept {

    if (frame == nullptr ||
        frame_size < kFrameSize) {
        return false;
    }

    PerceptionFrameHeader header{};

    std::memcpy(
        &header,
        frame,
        sizeof(header));

    if (header.magic != PerceptionFrameHeader::kMagic ||
        header.version != PerceptionFrameHeader::kVersion ||
        header.header_size != kHeaderSize ||
        header.width != kWidth ||
        header.height != kHeight ||
        header.timestamp_ns == 0 ||
        header.payload_size != kPayloadSize) {
        return false;
    }

    const std::uint8_t* payload =
        frame + kHeaderSize;

    const std::uint8_t* segmentation_data =
        payload;

    const std::uint8_t* confidence_data =
        segmentation_data + kSegmentationSize;

    const std::uint8_t* depth_data =
        confidence_data + kFloatArraySize;

    const std::uint8_t* roughness_data =
        depth_data + kFloatArraySize;

    output.width =
        static_cast<int>(header.width);

    output.height =
        static_cast<int>(header.height);

    output.timestamp_ns =
        header.timestamp_ns;

    output.segmentation.resize(kPixelCount);
    output.confidence.resize(kPixelCount);
    output.depth.resize(kPixelCount);
    output.roughness.resize(kPixelCount);

    for (std::size_t i = 0; i < kPixelCount; ++i) {

        const std::uint8_t semantic_id =
            segmentation_data[i];

        if (semantic_id > 3) {
            return false;
        }

        output.segmentation[i] =
            static_cast<SemanticClass>(
                semantic_id);
    }

    std::memcpy(
        output.confidence.data(),
        confidence_data,
        kFloatArraySize);

    std::memcpy(
        output.depth.data(),
        depth_data,
        kFloatArraySize);

    std::memcpy(
        output.roughness.data(),
        roughness_data,
        kFloatArraySize);

    for (std::size_t i = 0; i < kPixelCount; ++i) {

        if (!std::isfinite(output.confidence[i]) ||
            !std::isfinite(output.depth[i]) ||
            !std::isfinite(output.roughness[i])) {
            return false;
        }

        if (output.confidence[i] < 0.0f ||
            output.confidence[i] > 1.0f ||
            output.depth[i] < 0.0f ||
            output.depth[i] > 1.0f ||
            output.roughness[i] < 0.0f ||
            output.roughness[i] > 1.0f) {
            return false;
        }
    }

    return output.isValid();
}

}  // namespace ugv