#include "../transport/metric_map_reader.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    constexpr std::uint32_t width = 160;
    constexpr std::uint32_t height = 160;

    constexpr std::size_t pixel_count =
        static_cast<std::size_t>(width) * height;

    constexpr std::size_t frame_size =
        sizeof(ugv::MetricMapFrameHeader) +
        pixel_count +
        pixel_count;

    std::vector<std::uint8_t> frame(frame_size, 0);

    auto* header =
        reinterpret_cast<ugv::MetricMapFrameHeader*>(
            frame.data()
        );

    header->magic = ugv::MetricMapFrameHeader::kMagic;

    header->version = ugv::MetricMapFrameHeader::kVersion;

    header->header_size = sizeof(ugv::MetricMapFrameHeader);
    header->width = width;
    header->height = height;
    header->sequence = 7;
    header->timestamp_ns = 123456789;
    header->payload_size =
        ugv::MetricMapFrameHeader::kExpectedPayloadSize;

    std::uint8_t* payload =
        frame.data() +
        sizeof(ugv::MetricMapFrameHeader);

    payload[0] = 25;
    payload[1] = 65;
    payload[2] = 100;

    payload[pixel_count + 0] = 1;
    payload[pixel_count + 1] = 1;
    payload[pixel_count + 2] = 0;

    ugv::MetricMapReader reader;
    ugv::MetricMapFrame output;

    const bool decoded =
        reader.decode(
            frame.data(),
            frame.size(),
            output
        );

    assert(decoded);
    assert(output.isValid());

    assert(output.width == 160);
    assert(output.height == 160);
    assert(output.sequence == 7);
    assert(output.timestamp_ns == 123456789);

    assert(output.cost_map[0] == 25);
    assert(output.cost_map[1] == 65);
    assert(output.cost_map[2] == 100);

    assert(output.observed[0] == 1);
    assert(output.observed[1] == 1);
    assert(output.observed[2] == 0);

    std::cout
        << "METRIC MAP READER: PASS\n";

    return 0;
}