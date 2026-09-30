#pragma once

#include "perception_frame_header.hpp"

#include <cstddef>
#include <cstdint>

namespace ugv {

class PerceptionFrameHeaderValidator {
public:
    static bool isValid(const PerceptionFrameHeader& header) noexcept {
        if (header.magic != PerceptionFrameHeader::kMagic) {
            return false;
        }

        if (header.version != PerceptionFrameHeader::kVersion) {
            return false;
        }

        if (header.header_size != sizeof(PerceptionFrameHeader)) {
            return false;
        }

        if (header.width != 512 || header.height != 512) {
            return false;
        }

        if (header.timestamp_ns == 0) {
            return false;
        }

        if (header.payload_size !=
            PerceptionFrameHeader::kExpectedPayloadSize) {
            return false;
        }

        return true;
    }
};

} // namespace ugv