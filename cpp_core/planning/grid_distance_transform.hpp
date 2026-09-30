#pragma once

#include "../traversability/cost_map.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace ugv {

class GridDistanceTransform {
public:
    GridDistanceTransform() = default;

    bool compute(
        const CostMap& map,
        std::uint8_t blocked_threshold
    ) noexcept {

        if (map.width() <= 0 ||
            map.height() <= 0) {
            return false;
        }

        if (!std::isfinite(map.resolution()) ||
            map.resolution() <= 0.0) {
            return false;
        }

        const int width = map.width();
        const int height = map.height();

        const std::size_t cell_count =
            static_cast<std::size_t>(width) *
            static_cast<std::size_t>(height);

        distances_.assign(
            cell_count,
            std::numeric_limits<double>::infinity());

        bool has_blocked_cell = false;

        /*
         * Each blocked cell starts with squared distance 0.
         * Every other cell starts at infinity.
         */
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {

                const std::size_t index =
                    indexOf(width, x, y);

                if (map.getCost(x, y) >=
                    blocked_threshold) {

                    distances_[index] = 0.0;
                    has_blocked_cell = true;
                }
            }
        }

        /*
         * No obstacles means infinite clearance everywhere.
         */
        if (!has_blocked_cell) {
            return true;
        }

        /*
         * Exact squared Euclidean distance transform.
         *
         * First transform every column.
         */
        std::vector<double> column_distances(
            cell_count,
            std::numeric_limits<double>::infinity());

        std::vector<double> f(
            static_cast<std::size_t>(height));

        std::vector<double> d(
            static_cast<std::size_t>(height));

        for (int x = 0; x < width; ++x) {

            for (int y = 0; y < height; ++y) {
                f[static_cast<std::size_t>(y)] =
                    distances_[indexOf(width, x, y)];
            }

            distanceTransform1D(
                f,
                d);

            for (int y = 0; y < height; ++y) {
                column_distances[
                    indexOf(width, x, y)] =
                    d[static_cast<std::size_t>(y)];
            }
        }

        /*
         * Then transform every row.
         */
        std::vector<double> row_input(
            static_cast<std::size_t>(width));

        std::vector<double> row_output(
            static_cast<std::size_t>(width));

        for (int y = 0; y < height; ++y) {

            for (int x = 0; x < width; ++x) {
                row_input[
                    static_cast<std::size_t>(x)] =
                    column_distances[
                        indexOf(width, x, y)];
            }

            distanceTransform1D(
                row_input,
                row_output);

            for (int x = 0; x < width; ++x) {
                distances_[
                    indexOf(width, x, y)] =
                    std::sqrt(
                        std::max(
                            0.0,
                            row_output[
                                static_cast<std::size_t>(x)]));
            }
        }

        return true;
    }

    double clearanceMeters(
        const CostMap& map,
        int cell_x,
        int cell_y
    ) const noexcept {

        if (cell_x < 0 ||
            cell_x >= map.width() ||
            cell_y < 0 ||
            cell_y >= map.height()) {
            return 0.0;
        }

        const std::size_t expected_size =
            static_cast<std::size_t>(map.width()) *
            static_cast<std::size_t>(map.height());

        if (distances_.size() != expected_size) {
            return 0.0;
        }

        const double distance_cells =
            distances_[
                indexOf(
                    map.width(),
                    cell_x,
                    cell_y)];

        if (!std::isfinite(distance_cells)) {
            return std::numeric_limits<double>::infinity();
        }

        return distance_cells *
               map.resolution();
    }

private:
    std::vector<double> distances_;

    static std::size_t indexOf(
        int width,
        int x,
        int y
    ) noexcept {
        return
            static_cast<std::size_t>(y) *
            static_cast<std::size_t>(width) +
            static_cast<std::size_t>(x);
    }

    /*
     * Felzenszwalb-Huttenlocher exact 1D
     * squared Euclidean distance transform.
     *
     * Input:
     *     f[q] = 0 for obstacle sites,
     *            infinity otherwise.
     *
     * Output:
     *     d[x] = min_q ((x-q)^2 + f[q])
     */
    static void distanceTransform1D(
        const std::vector<double>& input,
        std::vector<double>& output
    ) noexcept {

        const int n =
            static_cast<int>(input.size());

        if (n <= 0) {
            output.clear();
            return;
        }

        output.resize(
            static_cast<std::size_t>(n));

        std::vector<int> parabola_locations(
            static_cast<std::size_t>(n));

        std::vector<double> boundaries(
            static_cast<std::size_t>(n + 1));

        int k = 0;

        parabola_locations[0] = 0;
        boundaries[0] =
            -std::numeric_limits<double>::infinity();
        boundaries[1] =
            std::numeric_limits<double>::infinity();

        for (int q = 1; q < n; ++q) {

            double s = 0.0;

            while (true) {

                const int v =
                    parabola_locations[k];

                const double numerator =
                    (input[static_cast<std::size_t>(q)] +
                     static_cast<double>(q * q)) -
                    (input[static_cast<std::size_t>(v)] +
                     static_cast<double>(v * v));

                const double denominator =
                    2.0 *
                    static_cast<double>(q - v);

                s = numerator / denominator;

                if (s >
                    boundaries[
                        static_cast<std::size_t>(k)]) {
                    break;
                }

                if (k == 0) {
                    break;
                }

                --k;
            }

            ++k;

            parabola_locations[
                static_cast<std::size_t>(k)] = q;

            boundaries[
                static_cast<std::size_t>(k)] = s;

            boundaries[
                static_cast<std::size_t>(k + 1)] =
                std::numeric_limits<double>::infinity();
        }

        k = 0;

        for (int q = 0; q < n; ++q) {

            while (
                boundaries[
                    static_cast<std::size_t>(k + 1)] < q) {
                ++k;
            }

            const int v =
                parabola_locations[
                    static_cast<std::size_t>(k)];

            const double delta =
                static_cast<double>(q - v);

            output[
                static_cast<std::size_t>(q)] =
                delta * delta +
                input[
                    static_cast<std::size_t>(v)];
        }
    }
};

}  // namespace ugv