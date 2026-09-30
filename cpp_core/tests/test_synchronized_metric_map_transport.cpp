#include "../transport/synchronized_metric_map_transport.hpp"

#include <cassert>
#include <iostream>

int main() {
    ugv::SynchronizedMetricMapTransport transport;

    assert(transport.open());

    ugv::MetricMapFrame frame;

    assert(transport.consume(frame));
    assert(frame.isValid());

    assert(frame.width == 160);
    assert(frame.height == 160);
    assert(frame.timestamp_ns == 123456789);

    assert(frame.cost_map[10 * 160 + 20] == 25);
    assert(frame.cost_map[30 * 160 + 40] == 65);

    assert(frame.observed[10 * 160 + 20] == 1);
    assert(frame.observed[30 * 160 + 40] == 1);
    assert(frame.observed[50 * 160 + 60] == 0);

    std::cout << "PYTHON -> C++ SYNCHRONIZED METRIC MAP: PASS\n";

    return 0;
}