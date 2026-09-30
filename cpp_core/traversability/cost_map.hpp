#pragma once

#include <cstdint>
#include <vector>

namespace ugv {

struct CostMapMetadata {
    double resolution_m = 0.0;
    std::uint64_t timestamp_ns = 0;
    double origin_x_m = 0.0;
    double origin_y_m = 0.0;
};

class CostMap {
public:
    CostMap(int width, int height);

    void reset();

    void setCost(int x, int y, std::uint8_t cost);

    std::uint8_t getCost(int x, int y) const;

    int width() const;
    int height() const;

    // Physical size represented by one cell, in metres.
    void setResolution(double resolution_m);

    double resolution() const noexcept;

    // Monotonic timestamp associated with this map.
    void setTimestamp(std::uint64_t timestamp_ns);

    std::uint64_t timestamp() const noexcept;

    // Map origin in the UGV/world frame, metres.
    void setOrigin(double x_m, double y_m);

    double originX() const noexcept;
    double originY() const noexcept;

    // Read-only snapshot of map metadata.
    CostMapMetadata metadata() const noexcept;

private:
    int width_;
    int height_;

    std::vector<std::uint8_t> data_;

    double resolution_m_ = 0.0;
    std::uint64_t timestamp_ns_ = 0;

    double origin_x_m_ = 0.0;
    double origin_y_m_ = 0.0;
};

}  // namespace ugv