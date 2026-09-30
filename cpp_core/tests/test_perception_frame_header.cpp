#include "../common/perception_frame_header_validator.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    ugv::PerceptionFrameHeader header;

    header.width = 512;
    header.height = 512;
    header.sequence = 1;
    header.timestamp_ns = 1000000;
    header.payload_size =
        static_cast<std::uint32_t>(
            ugv::PerceptionFrameHeader::kExpectedPayloadSize);

    assert(ugv::PerceptionFrameHeaderValidator::isValid(header));

    header.magic = 0;
    assert(!ugv::PerceptionFrameHeaderValidator::isValid(header));

    header.magic = ugv::PerceptionFrameHeader::kMagic;
    header.version = 99;
    assert(!ugv::PerceptionFrameHeaderValidator::isValid(header));

    header.version = ugv::PerceptionFrameHeader::kVersion;
    header.timestamp_ns = 0;
    assert(!ugv::PerceptionFrameHeaderValidator::isValid(header));

    header.timestamp_ns = 1000000;
    header.payload_size = 0;
    assert(!ugv::PerceptionFrameHeaderValidator::isValid(header));

    std::cout << "Perception frame header validation: PASS\n";

    return 0;
}