#include "metric_map_reader.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace ugv {

bool MetricMapReader::decode(
    const std::uint8_t* frame,
    std::size_t frame_size,
    MetricMapFrame& output
) const noexcept {

    output = MetricMapFrame{};

    if (frame == nullptr) {
        return false;
    }

    if (frame_size <
        sizeof(MetricMapFrameHeader)) {
        return false;
    }

    MetricMapFrameHeader header{};

    std::memcpy(
        &header,
        frame,
        sizeof(header)
    );

    if (header.magic !=
        MetricMapFrameHeader::kMagic) {
        return false;
    }

    if (header.version !=
        MetricMapFrameHeader::kVersion) {
        return false;
    }

    if (header.header_size !=
        sizeof(MetricMapFrameHeader)) {
        return false;
    }

    if (header.width != 160 ||
        header.height != 160) {
        return false;
    }

    if (header.sequence == 0 ||
        header.timestamp_ns == 0) {
        return false;
    }

    if (header.payload_size !=
        MetricMapFrameHeader::
            kExpectedPayloadSize) {
        return false;
    }

    const std::size_t expected_size =
        sizeof(MetricMapFrameHeader) +
        MetricMapFrameHeader::
            kExpectedPayloadSize;

    if (frame_size < expected_size) {
        return false;
    }

    const std::size_t pixel_count =
        static_cast<std::size_t>(
            header.width
        ) *
        static_cast<std::size_t>(
            header.height
        );

    const std::uint8_t* payload =
        frame +
        sizeof(MetricMapFrameHeader);

    const std::uint8_t* cost_data =
        payload;

    const std::uint8_t* observed_data =
        payload + pixel_count;

    output.width = header.width;
    output.height = header.height;
    output.sequence = header.sequence;
    output.timestamp_ns = header.timestamp_ns;

    output.cost_map.assign(
        cost_data,
        cost_data + pixel_count
    );

    output.observed.assign(
        observed_data,
        observed_data + pixel_count
    );

    for (std::size_t i = 0;
         i < pixel_count;
         ++i) {

        if (output.cost_map[i] > 100) {
            output = MetricMapFrame{};
            return false;
        }

        if (output.observed[i] > 1) {
            output = MetricMapFrame{};
            return false;
        }
    }

    return output.isValid();
}

}  // namespace ugv