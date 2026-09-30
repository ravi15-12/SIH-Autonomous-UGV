#pragma once

#include <cstddef>
#include <cstdint>

namespace ugv {

#pragma pack(push, 1)

struct MetricMapFrameHeader {
    static constexpr std::uint32_t kMagic = 0x5547564D;
    static constexpr std::uint16_t kVersion = 1;

    std::uint32_t magic = kMagic;
    std::uint16_t version = kVersion;
    std::uint16_t header_size = 40;

    std::uint32_t width = 0;
    std::uint32_t height = 0;

    std::uint64_t sequence = 0;
    std::uint64_t timestamp_ns = 0;

    std::uint32_t payload_size = 0;

    std::uint32_t reserved = 0;

    static constexpr std::size_t kExpectedPayloadSize =
        (160ULL * 160ULL * sizeof(std::uint8_t)) +
        (160ULL * 160ULL * sizeof(std::uint8_t));
};

#pragma pack(pop)

static_assert(sizeof(MetricMapFrameHeader) == 40,
              "Unexpected MetricMapFrameHeader size");

static_assert(offsetof(MetricMapFrameHeader, magic) == 0);
static_assert(offsetof(MetricMapFrameHeader, version) == 4);
static_assert(offsetof(MetricMapFrameHeader, header_size) == 6);
static_assert(offsetof(MetricMapFrameHeader, width) == 8);
static_assert(offsetof(MetricMapFrameHeader, height) == 12);
static_assert(offsetof(MetricMapFrameHeader, sequence) == 16);
static_assert(offsetof(MetricMapFrameHeader, timestamp_ns) == 24);
static_assert(offsetof(MetricMapFrameHeader, payload_size) == 32);
static_assert(offsetof(MetricMapFrameHeader, reserved) == 36);

}  // namespace ugv