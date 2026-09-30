#include "../transport/synchronized_perception_transport.hpp"
#include "../traversability/cost_map.hpp"
#include "../traversability/traversability_config.hpp"
#include "../traversability/traversability_engine.hpp"

#include <cstdint>
#include <iostream>

int main() {
    ugv::SynchronizedPerceptionTransport transport;

    if (!transport.open(
            "ugv_perception_frame",
            "ugv_perception_ready",
            "ugv_perception_free")) {
        std::cerr
            << "FAILED: unable to open synchronized transport\n";
        return 1;
    }

    ugv::PerceptionFrame perception;

    if (!transport.consume(perception)) {
        std::cerr
            << "FAILED: unable to consume perception frame\n";
        transport.close();
        return 1;
    }

    if (!perception.isValid()) {
        std::cerr
            << "FAILED: received perception frame is invalid\n";
        transport.close();
        return 1;
    }

    ugv::TraversabilityConfig config;

    ugv::TraversabilityEngine engine(config);

    ugv::CostMap cost_map(
        perception.width,
        perception.height);

    const std::uint64_t now_timestamp_ns =
        perception.timestamp_ns;

    if (!engine.compute(
            perception,
            now_timestamp_ns,
            cost_map)) {
        std::cerr
            << "FAILED: traversability computation failed\n";
        transport.close();
        return 1;
    }

    if (cost_map.width() != perception.width ||
        cost_map.height() != perception.height) {
        std::cerr
            << "FAILED: cost map dimensions mismatch\n";
        transport.close();
        return 1;
    }

    if (cost_map.timestamp() !=
        perception.timestamp_ns) {
        std::cerr
            << "FAILED: cost map timestamp mismatch\n";
        transport.close();
        return 1;
    }

    bool has_nonzero_cost = false;
    bool has_blocked_cost = false;

    for (int y = 0; y < cost_map.height(); ++y) {
        for (int x = 0; x < cost_map.width(); ++x) {
            const std::uint8_t cost =
                cost_map.getCost(x, y);

            if (cost > 0) {
                has_nonzero_cost = true;
            }

            if (cost == config.blocked_threshold) {
                has_blocked_cost = true;
            }

            if (cost > config.blocked_threshold) {
                std::cerr
                    << "FAILED: cost exceeds blocked threshold\n";
                transport.close();
                return 1;
            }
        }
    }

    if (!has_nonzero_cost) {
        std::cerr
            << "FAILED: cost map contains no nonzero costs\n";
        transport.close();
        return 1;
    }

    if (!has_blocked_cost) {
        std::cerr
            << "FAILED: live perception produced no blocked cells\n";
        transport.close();
        return 1;
    }

    transport.close();

    std::cout
        << "LIVE PERCEPTION -> TRAVERSABILITY: PASS\n"
        << "Perception: "
        << perception.width
        << " x "
        << perception.height
        << "\n"
        << "CostMap: "
        << cost_map.width()
        << " x "
        << cost_map.height()
        << "\n"
        << "Timestamp: "
        << cost_map.timestamp()
        << "\n";

    return 0;
}