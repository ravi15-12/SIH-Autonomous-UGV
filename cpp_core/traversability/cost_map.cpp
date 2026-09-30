#include "cost_map.hpp"

#include <stdexcept>
#include <cmath>
#include <stdexcept>

namespace ugv {

CostMap::CostMap(int width, int height)
    : width_(width),
      height_(height),
      data_(static_cast<std::size_t>(width) *
            static_cast<std::size_t>(height),
            0) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument(
            "CostMap dimensions must be positive"
        );
    }
}

void CostMap::reset() {
    std::fill(data_.begin(), data_.end(), 0);
}

void CostMap::setCost(int x, int y, std::uint8_t cost) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        throw std::out_of_range("CostMap coordinate out of bounds");
    }

    data_[static_cast<std::size_t>(y) *
          static_cast<std::size_t>(width_) +
          static_cast<std::size_t>(x)] = cost;
}

std::uint8_t CostMap::getCost(int x, int y) const {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        throw std::out_of_range("CostMap coordinate out of bounds");
    }

    return data_[static_cast<std::size_t>(y) *
                 static_cast<std::size_t>(width_) +
                 static_cast<std::size_t>(x)];
}

int CostMap::width() const {
    return width_;
}

int CostMap::height() const {
    return height_;
}
void CostMap::setResolution(double resolution_m) {
    if (!std::isfinite(resolution_m) || resolution_m <= 0.0) {
        throw std::invalid_argument(
            "CostMap resolution must be finite and positive"
        );
    }

    resolution_m_ = resolution_m;
}

double CostMap::resolution() const noexcept {
    return resolution_m_;
}

void CostMap::setTimestamp(std::uint64_t timestamp_ns) {
    if (timestamp_ns == 0) {
        throw std::invalid_argument(
            "CostMap timestamp must be non-zero"
        );
    }

    timestamp_ns_ = timestamp_ns;
}

std::uint64_t CostMap::timestamp() const noexcept {
    return timestamp_ns_;
}
void CostMap::setOrigin(double x_m, double y_m) {
    if (!std::isfinite(x_m) || !std::isfinite(y_m)) {
        throw std::invalid_argument(
            "CostMap origin must be finite"
        );
    }

    origin_x_m_ = x_m;
    origin_y_m_ = y_m;
}

double CostMap::originX() const noexcept {
    return origin_x_m_;
}

double CostMap::originY() const noexcept {
    return origin_y_m_;
}

}  // namespace ugv
