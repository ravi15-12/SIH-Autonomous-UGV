#pragma once

#include <cstddef>
#include <cstdint>

namespace ugv {

#pragma pack(push, 1)

struct PerceptionFrameHeader {
    static constexpr std::uint32_t kMagic = 0x55475650;
    static constexpr std::uint16_t kVersion = 1;

    std::uint32_t magic = kMagic;
    std::uint16_t version = kVersion;
    std::uint16_t header_size = 36;

    std::uint32_t width = 0;
    std::uint32_t height = 0;

    std::uint64_t sequence = 0;
    std::uint64_t timestamp_ns = 0;

    std::uint32_t payload_size = 0;

    static constexpr std::size_t kExpectedPayloadSize =
        (512ULL * 512ULL * sizeof(std::uint8_t)) +
        (512ULL * 512ULL * sizeof(float)) +
        (512ULL * 512ULL * sizeof(float)) +
        (512ULL * 512ULL * sizeof(float));
};

#pragma pack(pop)

static_assert(sizeof(PerceptionFrameHeader) == 36,
              "Unexpected PerceptionFrameHeader size");

static_assert(offsetof(PerceptionFrameHeader, magic) == 0);
static_assert(offsetof(PerceptionFrameHeader, version) == 4);
static_assert(offsetof(PerceptionFrameHeader, header_size) == 6);
static_assert(offsetof(PerceptionFrameHeader, width) == 8);
static_assert(offsetof(PerceptionFrameHeader, height) == 12);
static_assert(offsetof(PerceptionFrameHeader, sequence) == 16);
static_assert(offsetof(PerceptionFrameHeader, timestamp_ns) == 24);
static_assert(offsetof(PerceptionFrameHeader, payload_size) == 32);

} // namespace ugv