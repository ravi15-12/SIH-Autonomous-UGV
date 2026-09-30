#pragma once

#include "pose_frame_reader.hpp"

#include <cstddef>

namespace ugv {

class SynchronizedPoseTransport {
public:
    static constexpr const char* kSharedMemoryName =
        "/ugv_pose";

    static constexpr const char* kReadySemaphoreName =
        "/ugv_pose_ready";

    static constexpr const char* kFreeSemaphoreName =
        "/ugv_pose_free";

    SynchronizedPoseTransport() = default;

    ~SynchronizedPoseTransport();

    SynchronizedPoseTransport(
        const SynchronizedPoseTransport&
    ) = delete;

    SynchronizedPoseTransport& operator=(
        const SynchronizedPoseTransport&
    ) = delete;

    bool open();

    bool consume(PoseFrame& output);

    void close();

private:
    int shm_fd_ = -1;

    void* mapped_memory_ = nullptr;

    std::size_t mapped_size_ = 0;

    void* ready_semaphore_ = nullptr;

    void* free_semaphore_ = nullptr;

    PoseFrameReader reader_;
};

}  // namespace ugv