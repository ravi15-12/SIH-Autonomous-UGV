#pragma once

#include "metric_map_reader.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace ugv {

class SynchronizedMetricMapTransport {
public:
    static constexpr const char* kSharedMemoryName =
        "/ugv_metric_map";

    static constexpr const char* kReadySemaphoreName =
        "/ugv_metric_map_ready";

    static constexpr const char* kFreeSemaphoreName =
        "/ugv_metric_map_free";

    SynchronizedMetricMapTransport() = default;

    ~SynchronizedMetricMapTransport();

    SynchronizedMetricMapTransport(
        const SynchronizedMetricMapTransport&
    ) = delete;

    SynchronizedMetricMapTransport& operator=(
        const SynchronizedMetricMapTransport&
    ) = delete;

    bool open();

    bool consume(
        MetricMapFrame& output
    );

    void close();

private:
    int shm_fd_ = -1;
    void* mapped_memory_ = nullptr;
    std::size_t mapped_size_ = 0;

    void* ready_semaphore_ = nullptr;
    void* free_semaphore_ = nullptr;

    MetricMapReader reader_;
};

}  // namespace ugv