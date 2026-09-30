#include "../transport/synchronized_perception_transport.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>

int main() {
    ugv::SynchronizedPerceptionTransport transport;

    if (!transport.open(
            "ugv_perception_frame",
            "ugv_perception_ready",
            "ugv_perception_free")) {
        std::cerr << "FAILED: unable to open synchronized transport\n";
        return 1;
    }

    ugv::PerceptionFrame frame;

    if (!transport.consume(frame)) {
        std::cerr << "FAILED: unable to consume perception frame\n";
        transport.close();
        return 1;
    }

    if (!frame.isValid()) {
        std::cerr << "FAILED: invalid perception frame\n";
        transport.close();
        return 1;
    }

    if (frame.width != 512 ||
        frame.height != 512) {
        std::cerr << "FAILED: unexpected dimensions\n";
        transport.close();
        return 1;
    }

    bool has_traversable = false;
    bool has_non_traversable = false;
    bool has_obstacle = false;

    for (std::size_t i = 0; i < frame.segmentation.size(); ++i) {
        const auto semantic = frame.segmentation[i];

        if (semantic == ugv::SemanticClass::Traversable) {
            has_traversable = true;
        } else if (
            semantic == ugv::SemanticClass::NonTraversable) {
            has_non_traversable = true;
        } else if (
            semantic == ugv::SemanticClass::Obstacle) {
            has_obstacle = true;
        }

        if (!std::isfinite(frame.confidence[i]) ||
            !std::isfinite(frame.depth[i]) ||
            !std::isfinite(frame.roughness[i])) {
            std::cerr
                << "FAILED: non-finite perception value\n";
            transport.close();
            return 1;
        }

        if (frame.confidence[i] < 0.0f ||
            frame.confidence[i] > 1.0f ||
            frame.depth[i] < 0.0f ||
            frame.depth[i] > 1.0f ||
            frame.roughness[i] < 0.0f ||
            frame.roughness[i] > 1.0f) {
            std::cerr
                << "FAILED: perception value outside [0, 1]\n";
            transport.close();
            return 1;
        }
    }

    if (!has_traversable ||
        !has_non_traversable ||
        !has_obstacle) {
        std::cerr
            << "FAILED: expected semantic classes not present\n";
        transport.close();
        return 1;
    }

    const auto confidence_min =
        *std::min_element(
            frame.confidence.begin(),
            frame.confidence.end());

    const auto confidence_max =
        *std::max_element(
            frame.confidence.begin(),
            frame.confidence.end());

    const auto depth_min =
        *std::min_element(
            frame.depth.begin(),
            frame.depth.end());

    const auto depth_max =
        *std::max_element(
            frame.depth.begin(),
            frame.depth.end());

    const auto roughness_min =
        *std::min_element(
            frame.roughness.begin(),
            frame.roughness.end());

    const auto roughness_max =
        *std::max_element(
            frame.roughness.begin(),
            frame.roughness.end());

    transport.close();

    std::cout
        << "PERCEPTION DATA CONTRACT: PASS\n"
        << "Confidence: "
        << confidence_min
        << " to "
        << confidence_max
        << "\n"
        << "Depth: "
        << depth_min
        << " to "
        << depth_max
        << "\n"
        << "Roughness: "
        << roughness_min
        << " to "
        << roughness_max
        << "\n";

    return 0;
}